#!/usr/bin/env python3
"""
Metal Kernel Catalog Builder

This tool compiles Metal kernel source files and generates a JSON catalog
with binary data encoded in Base64 format.

Requirements:
- Xcode with Metal development tools (macOS only)
- xcrun with metal and metallib tools available
- Python 3.6+

Usage:
    python metal_catalog_builder.py <kernel_file.metal> [options]
"""

import os
import sys
import json
import base64
import hashlib
import subprocess
import argparse
import re
import tempfile
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from datetime import datetime
import platform
import plistlib

class MetalKernelParser:
    """Parser for extracting kernel information from Metal source files using preprocessor."""
    
    def __init__(self, source_path: str, compiler_path: str = None):
        self.source_path = Path(source_path)
        self.compiler_path = compiler_path or "xcrun"
        with open(source_path, 'r', encoding='utf-8') as f:
            self.source_content = f.read()
        
        # Generate preprocessed output for accurate type resolution
        self.preprocessed_content = self._preprocess_source()
    
    def _preprocess_source(self) -> str:
        """Run source through preprocessor to resolve types and macros."""
        try:
            # Create a modified version that includes debug information
            with tempfile.NamedTemporaryFile(mode='w', suffix='.metal', delete=False) as temp_source:
                # Add debug macros and Metal headers
                debug_content = self._add_signature_capture_macros() + '\n' + self.source_content
                temp_source.write(debug_content)
                temp_source_path = temp_source.name
            
            # Run preprocessor with Metal includes
            cmd = [
                self.compiler_path,
                'metal',
                '-E',  # Preprocess only
                '-std=metal2.4',  # Use latest Metal standard
                '-ffast-math',
                '-I/System/Library/Frameworks/Metal.framework/Headers',
                temp_source_path
            ]
            
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            if result.returncode == 0:
                return result.stdout
            else:
                print(f"Preprocessor warning: {result.stderr}")
                return self.source_content  # Fallback to original
                
        except Exception as e:
            print(f"Preprocessing failed: {e}")
            return self.source_content
        finally:
            # Clean up temp file
            if 'temp_source_path' in locals() and os.path.exists(temp_source_path):
                os.unlink(temp_source_path)
    
    def _add_signature_capture_macros(self) -> str:
        """Add macros to capture function signatures during preprocessing."""
        return '''
// Signature capture macros for Metal
#define CAPTURE_KERNEL_START(name) /* KERNEL_START: name */
#define CAPTURE_KERNEL_END(name) /* KERNEL_END: name */
#define CAPTURE_PARAM(type, name) /* PARAM: type name */

// Include common Metal headers
#include <metal_stdlib>
using namespace metal;

// Metal-specific definitions for better parsing
#ifndef __METAL_VERSION__
#define __METAL_VERSION__ 240
#endif
'''
    
    def extract_kernels(self) -> List[Dict]:
        """Extract kernel function information using preprocessed source."""
        kernels = []
        
        # Look for kernel patterns in both original and preprocessed content
        original_kernels = self._extract_from_original()
        preprocessed_kernels = self._extract_from_preprocessed()
        
        # Merge information from both sources
        kernels = self._merge_kernel_info(original_kernels, preprocessed_kernels)
        
        return kernels
    
    def _extract_from_original(self) -> List[Dict]:
        """Extract kernels from original source for structure."""
        kernels = []
        
        # Enhanced regex for Metal kernel detection
        kernel_patterns = [
            # Compute kernels with [[kernel]] attribute
            r'\[\[kernel\]\]\s*(?:(?:inline\s+)?(?:static\s+)?)?(?:void|[\w\s\*&<>:,]+)\s+(\w+)\s*\((.*?)\)',
            # Vertex shaders
            r'vertex\s+(?:[\w\s\*&<>:,]+)\s+(\w+)\s*\((.*?)\)',
            # Fragment shaders  
            r'fragment\s+(?:[\w\s\*&<>:,]+)\s+(\w+)\s*\((.*?)\)',
            # General function with stage_in/stage_out
            r'(?:vertex|fragment)\s+[\w\s\*&<>:,]*\s+(\w+)\s*\([^)]*\[\[stage_in\]\][^)]*\)'
        ]
        
        for pattern in kernel_patterns:
            for match in re.finditer(pattern, self.source_content, re.DOTALL | re.MULTILINE):
                kernel_name = match.group(1)
                params_str = match.group(2) if len(match.groups()) > 1 else ""
                
                # Get line number for better context
                line_num = self.source_content[:match.start()].count('\n') + 1
                
                # Determine kernel type
                kernel_type = self._determine_kernel_type(match.group(0))
                
                # Extract Metal-specific attributes
                attributes = self._extract_metal_attributes(match.group(0))
                
                kernel_info = {
                    "name": kernel_name,
                    "symbol": kernel_name,
                    "raw_params": params_str.strip(),
                    "line_number": line_num,
                    "description": self._extract_description(match.start(), kernel_name),
                    "return_type": self._extract_return_type(match.group(0)),
                    "kernel_type": kernel_type,
                    "attributes": attributes
                }
                
                # Avoid duplicates
                if not any(k["name"] == kernel_name for k in kernels):
                    kernels.append(kernel_info)
        
        return kernels
    
    def _determine_kernel_type(self, signature: str) -> str:
        """Determine the type of Metal kernel/shader."""
        if "[[kernel]]" in signature:
            return "compute"
        elif signature.strip().startswith("vertex"):
            return "vertex"
        elif signature.strip().startswith("fragment"):
            return "fragment"
        else:
            return "unknown"
    
    def _extract_return_type(self, signature: str) -> str:
        """Extract return type from function signature."""
        # Remove attributes and qualifiers
        clean_sig = re.sub(r'\[\[[^\]]+\]\]', '', signature)
        clean_sig = re.sub(r'\b(?:vertex|fragment|kernel|inline|static)\b', '', clean_sig)
        
        # Look for return type before function name
        match = re.search(r'^\s*([\w\s\*&<>:,]+?)\s+\w+\s*\(', clean_sig.strip())
        if match:
            return match.group(1).strip()
        
        return "void"
    
    def _extract_metal_attributes(self, signature: str) -> Dict:
        """Extract Metal-specific attributes from function signature."""
        attributes = {}
        
        # Extract all [[attribute]] patterns
        attr_pattern = r'\[\[([^\]]+)\]\]'
        for match in re.finditer(attr_pattern, signature):
            attr_content = match.group(1)
            
            if '(' in attr_content:
                # Attribute with parameters
                attr_match = re.match(r'(\w+)\((.*?)\)', attr_content)
                if attr_match:
                    attr_name = attr_match.group(1)
                    attr_params = attr_match.group(2)
                    attributes[attr_name] = attr_params
            else:
                # Simple attribute
                attributes[attr_content] = True
        
        return attributes
    
    def _extract_from_preprocessed(self) -> Dict[str, Dict]:
        """Extract type information from preprocessed content."""
        kernel_info = {}
        
        # Find function definitions in preprocessed output
        func_patterns = [
            r'(?:vertex|fragment)\s+([\w\s\*&<>:,]+)\s+(\w+)\s*\((.*?)\)\s*\{',
            r'void\s+(\w+)\s*\((.*?)\)\s*\{',  # Compute kernels after preprocessing
        ]
        
        for pattern in func_patterns:
            for match in re.finditer(pattern, self.preprocessed_content, re.DOTALL):
                if len(match.groups()) == 3:
                    return_type, func_name, params_str = match.groups()
                else:
                    func_name, params_str = match.groups()
                    return_type = "void"
                
                # Parse the fully expanded parameters
                parameters = self._parse_preprocessed_parameters(params_str)
                
                kernel_info[func_name] = {
                    "expanded_params": parameters,
                    "full_signature": match.group(0),
                    "return_type": return_type.strip()
                }
        
        return kernel_info
    
    def _parse_preprocessed_parameters(self, params_str: str) -> List[Dict]:
        """Parse parameters from preprocessed source with full type information."""
        if not params_str.strip():
            return []
        
        parameters = []
        
        # Split parameters, being careful about nested templates and Metal attributes
        params = self._smart_parameter_split(params_str)
        
        for param in params:
            param = param.strip()
            if not param:
                continue
            
            # Parse each parameter
            param_info = self._parse_single_parameter(param)
            if param_info:
                parameters.append(param_info)
        
        return parameters
    
    def _smart_parameter_split(self, params_str: str) -> List[str]:
        """Smart parameter splitting that handles Metal attributes and templates."""
        params = []
        current_param = ""
        bracket_depth = 0
        paren_depth = 0
        template_depth = 0
        
        i = 0
        while i < len(params_str):
            char = params_str[i]
            
            if char == ',' and bracket_depth == 0 and paren_depth == 0 and template_depth == 0:
                if current_param.strip():
                    params.append(current_param.strip())
                current_param = ""
            else:
                current_param += char
                
                if char == '[' and i + 1 < len(params_str) and params_str[i + 1] == '[':
                    bracket_depth += 1
                    current_param += '['
                    i += 1  # Skip the second [
                elif char == ']' and i + 1 < len(params_str) and params_str[i + 1] == ']':
                    bracket_depth -= 1
                    current_param += ']'
                    i += 1  # Skip the second ]
                elif char == '(':
                    paren_depth += 1
                elif char == ')':
                    paren_depth -= 1
                elif char == '<':
                    template_depth += 1
                elif char == '>':
                    template_depth -= 1
            
            i += 1
        
        if current_param.strip():
            params.append(current_param.strip())
        
        return params
    
    def _parse_single_parameter(self, param: str) -> Optional[Dict]:
        """Parse a single parameter declaration."""
        param = param.strip()
        if not param:
            return None
        
        # Extract Metal attributes first
        metal_attributes = {}
        attr_pattern = r'\[\[([^\]]+)\]\]'
        
        for match in re.finditer(attr_pattern, param):
            attr_content = match.group(1)
            if '(' in attr_content:
                # Attribute with value like [[buffer(0)]]
                attr_match = re.match(r'(\w+)\((.*?)\)', attr_content)
                if attr_match:
                    metal_attributes[attr_match.group(1)] = attr_match.group(2)
            else:
                metal_attributes[attr_content] = True
        
        # Remove attributes from parameter string
        clean_param = re.sub(attr_pattern, '', param).strip()
        
        # Handle Metal-specific qualifiers
        metal_qualifiers = []
        qualifier_patterns = [
            r'\b(const|device|threadgroup|thread|constant|texture2d|texture3d|sampler)\b',
            r'\b(half|half2|half3|half4|float2|float3|float4|int2|int3|int4|uint2|uint3|uint4)\b'
        ]
        
        for pattern in qualifier_patterns:
            for match in re.finditer(pattern, clean_param):
                metal_qualifiers.append(match.group(1))
        
        # Remove qualifiers for easier parsing
        for pattern in qualifier_patterns:
            clean_param = re.sub(pattern, '', clean_param)
        clean_param = clean_param.strip()
        
        # Split into tokens
        tokens = [t for t in clean_param.split() if t]
        if not tokens:
            return None
        
        # Last token is usually the parameter name
        param_name = tokens[-1]
        
        # Handle pointer/reference indicators attached to name
        while param_name.startswith('*') or param_name.startswith('&'):
            param_name = param_name[1:]
        
        # Remove array brackets from name
        param_name = re.sub(r'\[.*?\]', '', param_name)
        
        # Everything else is the type
        type_tokens = tokens[:-1]
        
        # Handle pointer/reference indicators
        pointer_level = 0
        is_reference = False
        
        # Count pointers and references
        type_str = ' '.join(type_tokens)
        pointer_level = type_str.count('*')
        is_reference = '&' in type_str
        
        # Clean up type string
        base_type = re.sub(r'[*&\s]+', ' ', type_str).strip()
        if not base_type and metal_qualifiers:
            base_type = metal_qualifiers[-1]  # Use last qualifier as base type
        
        # Reconstruct full type with qualifiers
        full_type_parts = metal_qualifiers + [base_type] if base_type else metal_qualifiers
        if pointer_level > 0:
            full_type_parts.append('*' * pointer_level)
        if is_reference:
            full_type_parts.append('&')
        
        full_type = ' '.join(full_type_parts)
        
        return {
            "name": param_name,
            "type": full_type,
            "base_type": base_type,
            "qualifiers": metal_qualifiers,
            "metal_attributes": metal_attributes,
            "pointer_level": pointer_level,
            "is_reference": is_reference,
            "description": f"Parameter {param_name} of type {full_type}",
            "optional": False
        }
    
    def _merge_kernel_info(self, original: List[Dict], preprocessed: Dict[str, Dict]) -> List[Dict]:
        """Merge kernel information from original and preprocessed sources."""
        kernels = []
        
        for orig_kernel in original:
            kernel_name = orig_kernel["name"]
            
            # Get preprocessed info if available
            preprocessed_info = preprocessed.get(kernel_name, {})
            
            # Use preprocessed parameters if available, otherwise fall back to simple parsing
            if "expanded_params" in preprocessed_info:
                parameters = preprocessed_info["expanded_params"]
                return_type = preprocessed_info.get("return_type", orig_kernel["return_type"])
            else:
                parameters = self._parse_parameters(orig_kernel["raw_params"])
                return_type = orig_kernel["return_type"]
            
            kernel_info = {
                "name": kernel_name,
                "symbol": kernel_name,
                "description": orig_kernel["description"],
                "return_type": return_type,
                "parameters": parameters,
                "calling_convention": "metal",
                "thread_safety": "conditionally-safe",
                "examples": [
                    {
                        "language": "metal",
                        "code": self._generate_example_code(kernel_name, orig_kernel["kernel_type"], parameters),
                        "description": f"Example usage of {kernel_name} {orig_kernel['kernel_type']} shader"
                    }
                ],
                "tags": ["metal", "gpu", orig_kernel["kernel_type"], "shader"],
                "since_version": "1.0.0",
                "metadata": {
                    "line_number": orig_kernel["line_number"],
                    "has_preprocessed_types": "expanded_params" in preprocessed_info,
                    "kernel_type": orig_kernel["kernel_type"],
                    "metal_attributes": orig_kernel["attributes"]
                }
            }
            
            kernels.append(kernel_info)
        
        return kernels
    
    def _generate_example_code(self, kernel_name: str, kernel_type: str, parameters: List[Dict]) -> str:
        """Generate example usage code based on kernel type."""
        if kernel_type == "compute":
            return f"""// Compute shader dispatch
id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
[encoder setComputePipelineState:pipelineState];
// Set buffers and textures
[encoder dispatchThreadgroups:MTLSizeMake(groups, 1, 1) 
         threadsPerThreadgroup:MTLSizeMake(threads, 1, 1)];
[encoder endEncoding];"""
        
        elif kernel_type == "vertex":
            return f"""// Vertex shader in render pass
id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:desc];
[encoder setRenderPipelineState:pipelineState];
[encoder setVertexBuffer:vertexBuffer offset:0 atIndex:0];
[encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:vertexCount];
[encoder endEncoding];"""
        
        elif kernel_type == "fragment":
            return f"""// Fragment shader (used with vertex shader in render pipeline)
MTLRenderPipelineDescriptor *desc = [[MTLRenderPipelineDescriptor alloc] init];
desc.vertexFunction = vertexFunction;
desc.fragmentFunction = fragmentFunction;  // {kernel_name}
desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;"""
        
        else:
            return f"// {kernel_name} usage example not available for type: {kernel_type}"
    
    def _parse_parameters(self, params_str: str) -> List[Dict]:
        """Fallback simple parameter parsing."""
        if not params_str.strip():
            return []
        
        parameters = []
        param_parts = [p.strip() for p in params_str.split(',') if p.strip()]
        
        for param in param_parts:
            param_info = self._parse_single_parameter(param)
            if param_info:
                parameters.append(param_info)
        
        return parameters
    
    def _extract_description(self, kernel_pos: int, kernel_name: str) -> str:
        """Extract description from comments before kernel."""
        lines = self.source_content[:kernel_pos].split('\n')
        
        # Look for comments before the kernel
        description_lines = []
        for line in reversed(lines[-10:]):  # Look at last 10 lines
            line = line.strip()
            if line.startswith('//') or line.startswith('*') or line.startswith('/*'):
                comment = re.sub(r'^[/\*\s]*', '', line)
                if comment:
                    description_lines.insert(0, comment)
            elif line and not line.startswith('*'):
                break
        
        if description_lines:
            return ' '.join(description_lines)
        else:
            return f"Metal {kernel_name} shader function"

class MetalCompiler:
    """Metal kernel compiler wrapper with preprocessor support."""
    
    def __init__(self):
        self.xcrun_path = self._find_xcrun()
        if not self.xcrun_path:
            raise RuntimeError("xcrun not found. Metal development requires Xcode on macOS.")
        
        # Check if we're on macOS
        if platform.system() != "Darwin":
            raise RuntimeError("Metal development is only supported on macOS.")
        
        # Get available Metal versions and SDK info
        self.sdk_info = self._get_sdk_info()
        self.metal_version = self._get_metal_version()
        self.available_targets = self._get_available_targets()
    
    def _find_xcrun(self) -> Optional[str]:
        """Find xcrun tool."""
        try:
            result = subprocess.run(['which', 'xcrun'], capture_output=True, text=True)
            if result.returncode == 0:
                return result.stdout.strip()
        except:
            pass
        
        return None
    
    def _get_sdk_info(self) -> Dict:
        """Get SDK information."""
        try:
            result = subprocess.run([self.xcrun_path, '--show-sdk-version'], 
                                  capture_output=True, text=True)
            sdk_version = result.stdout.strip() if result.returncode == 0 else "unknown"
            
            result = subprocess.run([self.xcrun_path, '--show-sdk-path'], 
                                  capture_output=True, text=True)
            sdk_path = result.stdout.strip() if result.returncode == 0 else "unknown"
            
            return {
                "version": sdk_version,
                "path": sdk_path
            }
        except:
            return {"version": "unknown", "path": "unknown"}
    
    def _get_metal_version(self) -> str:
        """Get Metal version."""
        try:
            # Check for Metal compiler version
            result = subprocess.run([self.xcrun_path, 'metal', '--version'], 
                                  capture_output=True, text=True)
            
            if result.returncode == 0:
                # Extract version from output
                version_match = re.search(r'metal-(\d+\.\d+)', result.stdout)
                if version_match:
                    return version_match.group(1)
            
            return "2.4"  # Default to Metal 2.4
        except:
            return "2.4"
    
    def _get_available_targets(self) -> List[str]:
        """Get available Metal target platforms."""
        targets = []
        
        # Standard Metal targets
        standard_targets = [
            "macos-metal2.4",
            "ios-metal2.4", 
            "tvos-metal2.4",
            "watchos-metal2.4"
        ]
        
        for target in standard_targets:
            if self._test_target_availability(target):
                targets.append(target)
        
        return targets if targets else ["macos-metal2.4"]  # Fallback
    
    def _test_target_availability(self, target: str) -> bool:
        """Test if a target platform is available."""
        try:
            # Create minimal test source
            test_source = """
#include <metal_stdlib>
using namespace metal;

[[kernel]]
void test_kernel() {
    // Empty test kernel
}
"""
            with tempfile.NamedTemporaryFile(mode='w', suffix='.metal', delete=False) as temp_file:
                temp_file.write(test_source)
                temp_source_path = temp_file.name
            
            # Try to compile for this target
            cmd = [self.xcrun_path, 'metal', '-target', target, '-c', temp_source_path]
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            return result.returncode == 0
            
        except:
            return False
        finally:
            if 'temp_source_path' in locals() and os.path.exists(temp_source_path):
                os.unlink(temp_source_path)
    
    def compile_to_air(self, source_path: str, output_path: str, 
                       target: str = "macos-metal2.4", optimization: str = "2") -> bool:
        """Compile Metal source to AIR (Apple Intermediate Representation)."""
        try:
            cmd = [
                self.xcrun_path,
                'metal',
                f'-target={target}',
                f'-O{optimization}',
                '-c',
                '-o', output_path,
                source_path
            ]
            
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            if result.returncode != 0:
                print(f"AIR compilation failed: {result.stderr}")
                return False
            
            return True
            
        except Exception as e:
            print(f"AIR compilation error: {e}")
            return False
    
    def compile_to_metallib(self, air_path: str, output_path: str) -> bool:
        """Compile AIR to metallib (Metal library)."""
        try:
            cmd = [
                self.xcrun_path,
                'metallib',
                '-o', output_path,
                air_path
            ]
            
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            if result.returncode != 0:
                print(f"Metallib compilation failed: {result.stderr}")
                return False
            
            return True
            
        except Exception as e:
            print(f"Metallib compilation error: {e}")
            return False
    
    def compile_to_binary(self, source_path: str, output_path: str, 
                         target: str = "macos-metal2.4", optimization: str = "2") -> bool:
        """Compile Metal source to metallib binary."""
        with tempfile.NamedTemporaryFile(suffix='.air', delete=False) as temp_air:
            temp_air_path = temp_air.name
        
        try:
            # First compile to AIR
            if not self.compile_to_air(source_path, temp_air_path, target, optimization):
                return False
            
            # Then compile AIR to metallib
            return self.compile_to_metallib(temp_air_path, output_path)
            
        finally:
            if os.path.exists(temp_air_path):
                os.unlink(temp_air_path)
    
    def preprocess_source(self, source_path: str, output_path: str = None) -> Optional[str]:
        """Preprocess source file and return preprocessed content."""
        try:
            cmd = [
                self.xcrun_path,
                'metal',
                '-E',  # Preprocess only
                f'-std=metal{self.metal_version}',
                source_path
            ]
            
            if output_path:
                cmd.extend(['-o', output_path])
            
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            if result.returncode == 0:
                if output_path:
                    return output_path
                else:
                    return result.stdout
            else:
                print(f"Preprocessing failed: {result.stderr}")
                return None
                
        except Exception as e:
            print(f"Preprocessing error: {e}")
            return None
    
    def get_metallib_info(self, metallib_path: str) -> Dict:
        """Get information about a compiled metallib."""
        try:
            # Use otool to get information about the metallib
            cmd = ['otool', '-l', metallib_path]
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            if result.returncode == 0:
                return {"otool_output": result.stdout}
            
        except Exception as e:
            print(f"Failed to analyze metallib: {e}")
        
        return {}
    
    def get_compiler_info(self) -> Dict:
        """Get compiler version and build information."""
        try:
            result = subprocess.run([self.xcrun_path, 'metal', '--version'], 
                                  capture_output=True, text=True)
            
            return {
                "compiler": "metal",
                "compiler_version": self.metal_version,
                "xcrun_version": self._get_xcrun_version(),
                "sdk_info": self.sdk_info,
                "available_targets": self.available_targets,
                "build_flags": ["-O2", "-std=metal2.4"],
                "optimization_level": "O2",
                "debug_symbols": False
            }
        except:
            return {
                "compiler": "metal",
                "compiler_version": self.metal_version,
                "available_targets": self.available_targets
            }
    
    def _get_xcrun_version(self) -> str:
        """Get xcrun version."""
        try:
            result = subprocess.run([self.xcrun_path, '--version'], 
                                  capture_output=True, text=True)
            return result.stdout.strip() if result.returncode == 0 else "unknown"
        except:
            return "unknown"

class CatalogBuilder:
    """Main catalog builder class."""
    
    def __init__(self):
        self.compiler = MetalCompiler()
    
    def build_catalog_entry(self, source_path: str, library_name: str = None,
                          target_platforms: List[str] = None, include_air: bool = False) -> Dict:
        """Build a complete catalog entry from Metal source."""
        
        if target_platforms is None:
            target_platforms = self.compiler.available_targets[:2]  # Use first 2 available
        
        source_file = Path(source_path)
        if library_name is None:
            library_name = source_file.stem
        
        # Parse kernels with preprocessor support
        parser = MetalKernelParser(source_path, self.compiler.xcrun_path)
        kernels = parser.extract_kernels()
        
        # Build architectures
        architectures = []
        
        for target in target_platforms:
            # Compile to metallib
            with tempfile.NamedTemporaryFile(suffix=".metallib", delete=False) as temp_metallib:
                temp_metallib_path = temp_metallib.name
            
            try:
                # Compile for this target
                success = self.compiler.compile_to_binary(
                    source_path, temp_metallib_path, target
                )
                
                if success and os.path.exists(temp_metallib_path):
                    # Read binary data
                    with open(temp_metallib_path, 'rb') as f:
                        binary_data = f.read()
                    
                    # Calculate checksum
                    sha256_hash = hashlib.sha256(binary_data).hexdigest()
                    
                    # Encode to base64
                    encoded_binary = base64.b64encode(binary_data).decode('utf-8')
                    
                    # Get metallib info
                    metallib_info = self.compiler.get_metallib_info(temp_metallib_path)
                    
                    arch_entry = {
                        "name": self._get_arch_name(target),
                        "platforms": [self._get_platform_name(target)],
                        "binary_format": "metallib",
                        "binary_data": encoded_binary,
                        "file_size": len(binary_data),
                        "checksum": {
                            "algorithm": "sha256",
                            "value": sha256_hash
                        },
                        "target_platform": target,
                        "metallib_info": metallib_info
                    }
                    
                    # Add AIR if requested
                    if include_air:
                        air_data = self._compile_to_air_data(source_path, target)
                        if air_data:
                            arch_entry["air_code"] = air_data
                    
                    architectures.append(arch_entry)
                
            finally:
                # Clean up temporary file
                if os.path.exists(temp_metallib_path):
                    os.unlink(temp_metallib_path)
        
        # Build complete library entry
        library_entry = {
            "id": f"metal-{library_name.lower()}",
            "name": library_name,
            "version": "1.0.0",
            "description": f"Metal shader library: {library_name}",
            "vendor": "Custom",
            "license": "MIT",
            "categories": ["graphics", "compute"],
            "architectures": architectures,
            "functions": kernels,
            "dependencies": [
                {
                    "name": "Metal Framework",
                    "version": f">={self.compiler.metal_version}",
                    "description": "Apple Metal graphics framework"
                },
                {
                    "name": "MetalKit",
                    "version": ">=1.0",
                    "optional": True,
                    "description": "Metal utility framework"
                }
            ],
            "build_info": self.compiler.get_compiler_info(),
            "metadata": {
                "source_file": str(source_file),
                "target_platforms": target_platforms,
                "framework": "Metal",
                "metal_version": self.compiler.metal_version,
                "preprocessor_used": True,
                "total_kernels": len(kernels),
                "kernel_names": [k["name"] for k in kernels],
                "kernel_types": list(set(k.get("metadata", {}).get("kernel_type", "unknown") for k in kernels)),
                "includes_air": include_air
            }
        }
        
        return library_entry
    
    def _get_arch_name(self, target: str) -> str:
        """Get architecture name from target string."""
        if "macos" in target:
            return "x86_64"  # or "arm64" for Apple Silicon
        elif "ios" in target:
            return "arm64"
        elif "tvos" in target:
            return "arm64"
        elif "watchos" in target:
            return "arm64"
        else:
            return "unknown"
    
    def _get_platform_name(self, target: str) -> str:
        """Get platform name from target string."""
        if "macos" in target:
            return "macos"
        elif "ios" in target:
            return "ios"
        elif "tvos" in target:
            return "tvos"
        elif "watchos" in target:
            return "watchos"
        else:
            return "unknown"
    
    def _compile_to_air_data(self, source_path: str, target: str) -> Optional[str]:
        """Compile source to AIR and return as base64 string."""
        with tempfile.NamedTemporaryFile(suffix=".air", delete=False) as temp_air:
            temp_air_path = temp_air.name
        
        try:
            success = self.compiler.compile_to_air(source_path, temp_air_path, target)
            
            if success and os.path.exists(temp_air_path):
                with open(temp_air_path, 'rb') as f:
                    air_data = f.read()
                
                # Encode AIR as base64 for JSON storage
                return base64.b64encode(air_data).decode('utf-8')
        
        except Exception as e:
            print(f"AIR compilation failed: {e}")
        
        finally:
            if os.path.exists(temp_air_path):
                os.unlink(temp_air_path)
        
        return None
    
    def build_full_catalog(self, libraries: List[Dict]) -> Dict:
        """Build complete catalog with metadata."""
        catalog = {
            "catalog": {
                "version": "1.0.0",
                "created": datetime.now().isoformat(),
                "updated": datetime.now().isoformat(),
                "description": "Metal Shader Library Catalog",
                "platform": "Apple Metal",
                "requires_macos": True
            },
            "libraries": libraries
        }
        
        return catalog

def main():
    parser = argparse.ArgumentParser(description="Build JSON catalog from Metal shader source")
    parser.add_argument("source", help="Metal shader source file (.metal)")
    parser.add_argument("-o", "--output", help="Output JSON file", 
                       default="metal_catalog.json")
    parser.add_argument("-n", "--name", help="Library name", default=None)
    parser.add_argument("-t", "--targets", nargs="+", 
                       help="Target platforms (e.g., macos-metal2.4, ios-metal2.4)", 
                       default=None)
    parser.add_argument("-v", "--verbose", action="store_true", 
                       help="Verbose output")
    parser.add_argument("--save-preprocessed", help="Save preprocessed source to file")
    parser.add_argument("--include", "-I", action="append", dest="include_paths",
                       help="Additional include paths", default=[])
    parser.add_argument("--include-air", action="store_true",
                       help="Include AIR (Apple Intermediate Representation) code")
    parser.add_argument("--metal-version", help="Metal language version to use",
                       default=None)
    
    args = parser.parse_args()
    
    # Check platform
    if platform.system() != "Darwin":
        print("Error: Metal development is only supported on macOS")
        sys.exit(1)
    
    if not os.path.exists(args.source):
        print(f"Error: Source file '{args.source}' not found")
        sys.exit(1)
    
    try:
        builder = CatalogBuilder()
        
        # Override Metal version if specified
        if args.metal_version:
            builder.compiler.metal_version = args.metal_version
        
        # Use detected targets if none specified
        target_platforms = args.targets or builder.compiler.available_targets[:2]
        
        if args.verbose:
            print(f"Processing {args.source}...")
            print(f"Target platforms: {target_platforms}")
            print(f"Metal version: {builder.compiler.metal_version}")
            print(f"SDK version: {builder.compiler.sdk_info['version']}")
            print(f"Available targets: {builder.compiler.available_targets}")
        
        # Save preprocessed output if requested
        if args.save_preprocessed:
            if args.verbose:
                print(f"Saving preprocessed source to {args.save_preprocessed}")
            builder.compiler.preprocess_source(args.source, args.save_preprocessed)
        
        # Build library entry
        library_entry = builder.build_catalog_entry(
            args.source, args.name, target_platforms, args.include_air
        )
        
        # Build full catalog
        catalog = builder.build_full_catalog([library_entry])
        
        # Write to file
        with open(args.output, 'w', encoding='utf-8') as f:
            json.dump(catalog, f, indent=2)
        
        if args.verbose:
            print(f"Catalog written to {args.output}")
            print(f"Found {len(library_entry['functions'])} shader functions:")
            
            # Group by shader type
            shader_types = {}
            for func in library_entry['functions']:
                shader_type = func.get('metadata', {}).get('kernel_type', 'unknown')
                if shader_type not in shader_types:
                    shader_types[shader_type] = []
                shader_types[shader_type].append(func)
            
            for shader_type, funcs in shader_types.items():
                print(f"  {shader_type.upper()} shaders ({len(funcs)}):")
                for func in funcs:
                    print(f"    - {func['name']}: {len(func['parameters'])} parameters")
                    if func.get('metadata', {}).get('has_preprocessed_types'):
                        print(f"      (using preprocessed types)")
                    
                    # Show Metal-specific attributes
                    metal_attrs = func.get('metadata', {}).get('metal_attributes', {})
                    if metal_attrs:
                        attrs_str = ", ".join([f"[[{k}]]" if v is True else f"[[{k}({v})]]" 
                                             for k, v in metal_attrs.items()])
                        print(f"      attributes: {attrs_str}")
            
            print(f"Built for {len(library_entry['architectures'])} platforms")
            
            # Show parameter type details with Metal-specific information
            print("\nParameter Details:")
            for func in library_entry['functions']:
                print(f"\n{func['name']} ({func.get('metadata', {}).get('kernel_type', 'unknown')}):")
                for param in func['parameters']:
                    type_info = param['type']
                    if 'base_type' in param:
                        extra_info = []
                        
                        # Show Metal attributes
                        if param.get('metal_attributes'):
                            metal_attrs = [f"[[{k}]]" if v is True else f"[[{k}({v})]]" 
                                         for k, v in param['metal_attributes'].items()]
                            extra_info.append(f"Metal attributes: {metal_attrs}")
                        
                        # Show qualifiers with Metal-specific highlighting
                        if param.get('qualifiers'):
                            metal_quals = [q for q in param['qualifiers'] if q in 
                                         ['device', 'threadgroup', 'thread', 'constant', 'texture2d', 'texture3d', 'sampler']]
                            other_quals = [q for q in param['qualifiers'] if q not in metal_quals]
                            if metal_quals:
                                extra_info.append(f"Metal qualifiers: {metal_quals}")
                            if other_quals:
                                extra_info.append(f"qualifiers: {other_quals}")
                        
                        if param.get('pointer_level', 0) > 0:
                            extra_info.append(f"pointer_level: {param['pointer_level']}")
                        if param.get('is_reference'):
                            extra_info.append("reference")
                        
                        detail = f" ({', '.join(extra_info)})" if extra_info else ""
                        print(f"    {param['name']}: {type_info}{detail}")
                    else:
                        print(f"    {param['name']}: {type_info}")
            
            # Show AIR information if included
            air_count = sum(1 for arch in library_entry['architectures'] if 'air_code' in arch)
            if air_count > 0:
                print(f"\nAIR code included for {air_count} platform(s)")
    
    except Exception as e:
        print(f"Error: {e}")
        if args.verbose:
            import traceback
            traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()
