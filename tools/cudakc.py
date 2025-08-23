#!/usr/bin/env python3
"""
CUDA Kernel Catalog Builder

This tool compiles CUDA kernel source files and generates a JSON catalog
with binary data encoded in Base64 format.

Requirements:
- CUDA Toolkit installed
- nvcc compiler available in PATH
- Python 3.6+

Usage:
    python cuda_catalog_builder.py <kernel_file.cu> [options]
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
import shlex

class CUDAKernelParser:
    """Parser for extracting kernel information from CUDA source files using preprocessor."""
    
    def __init__(self, source_path: str, compiler_path: str = None):
        self.source_path = Path(source_path)
        self.compiler_path = compiler_path or "nvcc"
        with open(source_path, 'r', encoding='utf-8') as f:
            self.source_content = f.read()
        
        # Generate preprocessed output for accurate type resolution
        self.preprocessed_content = self._preprocess_source()
    
    def _preprocess_source(self) -> str:
        """Run source through preprocessor to resolve types and macros."""
        try:
            # Create a modified version that includes debug information
            with tempfile.NamedTemporaryFile(mode='w', suffix='.cu', delete=False) as temp_source:
                # Add debug macros to capture function signatures
                debug_content = self._add_signature_capture_macros() + '\n' + self.source_content
                temp_source.write(debug_content)
                temp_source_path = temp_source.name
            
            # Run preprocessor with CUDA includes
            cmd = [
                self.compiler_path,
                '-E',  # Preprocess only
                '-I' + os.path.join(os.environ.get('CUDA_HOME', '/usr/local/cuda'), 'include'),
                '-D__CUDA_ARCH__=700',  # Default compute capability
                '-D__CUDACC__',
                '-D__NVCC__',
                '-x', 'cu',  # Treat as CUDA source
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
// Signature capture macros for CUDA
#define CAPTURE_KERNEL_START(name) /* KERNEL_START: name */
#define CAPTURE_KERNEL_END(name) /* KERNEL_END: name */
#define CAPTURE_PARAM(type, name) /* PARAM: type name */

// Include common CUDA headers
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

// Override __global__ to capture kernel signatures
#ifdef __global__
#undef __global__
#endif
#define __global__ CAPTURE_KERNEL_START(__func__) void __attribute__((global))
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
        
        # Enhanced regex for CUDA kernel detection
        # Supports __launch_bounds__, template kernels, and various qualifiers
        kernel_patterns = [
            # Standard __global__ kernels
            r'__global__\s+(?:(?:__launch_bounds__\s*\([^)]+\)\s+)?(?:inline\s+)?(?:static\s+)?)?(?:void|[\w\s\*&<>:,]+)\s+(\w+)\s*(?:<[^>]*>)?\s*\((.*?)\)\s*(?:\{|;)',
            # Template kernels
            r'template\s*<[^>]*>\s*__global__\s+(?:(?:__launch_bounds__\s*\([^)]+\)\s+)?(?:inline\s+)?(?:static\s+)?)?(?:void|[\w\s\*&<>:,]+)\s+(\w+)\s*\((.*?)\)\s*(?:\{|;)',
            # Device functions that might be kernels
            r'__device__\s+__global__\s+(?:void|[\w\s\*&<>:,]+)\s+(\w+)\s*\((.*?)\)\s*(?:\{|;)'
        ]
        
        for pattern in kernel_patterns:
            for match in re.finditer(pattern, self.source_content, re.DOTALL | re.MULTILINE):
                kernel_name = match.group(1)
                params_str = match.group(2)
                
                # Get line number for better context
                line_num = self.source_content[:match.start()].count('\n') + 1
                
                # Extract launch bounds if present
                launch_bounds = self._extract_launch_bounds(match.group(0))
                
                kernel_info = {
                    "name": kernel_name,
                    "symbol": kernel_name,
                    "raw_params": params_str.strip(),
                    "line_number": line_num,
                    "description": self._extract_description(match.start(), kernel_name),
                    "return_type": "void",
                    "launch_bounds": launch_bounds
                }
                
                # Avoid duplicates
                if not any(k["name"] == kernel_name for k in kernels):
                    kernels.append(kernel_info)
        
        return kernels
    
    def _extract_launch_bounds(self, kernel_signature: str) -> Optional[Dict]:
        """Extract __launch_bounds__ information if present."""
        launch_bounds_pattern = r'__launch_bounds__\s*\(\s*(\d+)(?:\s*,\s*(\d+))?\s*\)'
        match = re.search(launch_bounds_pattern, kernel_signature)
        
        if match:
            max_threads = int(match.group(1))
            min_blocks = int(match.group(2)) if match.group(2) else None
            
            return {
                "max_threads_per_block": max_threads,
                "min_blocks_per_multiprocessor": min_blocks
            }
        
        return None
    
    def _extract_from_preprocessed(self) -> Dict[str, Dict]:
        """Extract type information from preprocessed content."""
        kernel_info = {}
        
        # Find function definitions in preprocessed output
        func_patterns = [
            r'void\s+__attribute__\s*\(\s*\(\s*global\s*\)\s*\)\s+(\w+)\s*\((.*?)\)\s*\{',
            r'__global__\s+void\s+(\w+)\s*\((.*?)\)\s*\{'
        ]
        
        for pattern in func_patterns:
            for match in re.finditer(pattern, self.preprocessed_content, re.DOTALL):
                func_name = match.group(1)
                params_str = match.group(2)
                
                # Parse the fully expanded parameters
                parameters = self._parse_preprocessed_parameters(params_str)
                
                kernel_info[func_name] = {
                    "expanded_params": parameters,
                    "full_signature": match.group(0)
                }
        
        return kernel_info
    
    def _parse_preprocessed_parameters(self, params_str: str) -> List[Dict]:
        """Parse parameters from preprocessed source with full type information."""
        if not params_str.strip():
            return []
        
        parameters = []
        
        # Split parameters, being careful about nested templates and function pointers
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
        """Smart parameter splitting that handles nested templates and function pointers."""
        params = []
        current_param = ""
        paren_depth = 0
        template_depth = 0
        
        for char in params_str:
            if char == ',' and paren_depth == 0 and template_depth == 0:
                if current_param.strip():
                    params.append(current_param.strip())
                current_param = ""
            else:
                current_param += char
                
                if char == '(':
                    paren_depth += 1
                elif char == ')':
                    paren_depth -= 1
                elif char == '<':
                    template_depth += 1
                elif char == '>':
                    template_depth -= 1
        
        if current_param.strip():
            params.append(current_param.strip())
        
        return params
    
    def _parse_single_parameter(self, param: str) -> Optional[Dict]:
        """Parse a single parameter declaration."""
        param = param.strip()
        if not param:
            return None
        
        # Handle CUDA-specific qualifiers and memory space specifiers
        cuda_qualifiers = []
        qualifier_patterns = [
            r'\b(const|volatile|restrict|__restrict__|__restrict)\b',
            r'\b(__shared__|__constant__|__device__|__host__|__global__|__managed__)\b'
        ]
        
        for pattern in qualifier_patterns:
            for match in re.finditer(pattern, param):
                cuda_qualifiers.append(match.group(1))
        
        # Remove qualifiers for easier parsing
        clean_param = param
        for pattern in qualifier_patterns:
            clean_param = re.sub(pattern, '', clean_param)
        clean_param = clean_param.strip()
        
        # Split into tokens
        tokens = clean_param.split()
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
        
        # Reconstruct full type with qualifiers
        full_type_parts = cuda_qualifiers + [base_type]
        if pointer_level > 0:
            full_type_parts.append('*' * pointer_level)
        if is_reference:
            full_type_parts.append('&')
        
        full_type = ' '.join(full_type_parts)
        
        return {
            "name": param_name,
            "type": full_type,
            "base_type": base_type,
            "qualifiers": cuda_qualifiers,
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
            else:
                parameters = self._parse_parameters(orig_kernel["raw_params"])
            
            kernel_info = {
                "name": kernel_name,
                "symbol": kernel_name,
                "description": orig_kernel["description"],
                "return_type": "void",
                "parameters": parameters,
                "calling_convention": "device",
                "thread_safety": "conditionally-safe",
                "examples": [
                    {
                        "language": "cuda",
                        "code": f"// Launch {kernel_name} kernel\n{kernel_name}<<<blocks, threads>>>({', '.join(p['name'] for p in parameters)});",
                        "description": f"Basic launch of {kernel_name} kernel"
                    }
                ],
                "tags": ["cuda", "gpu", "kernel", "parallel"],
                "since_version": "1.0.0",
                "metadata": {
                    "line_number": orig_kernel["line_number"],
                    "has_preprocessed_types": "expanded_params" in preprocessed_info,
                    "launch_bounds": orig_kernel.get("launch_bounds")
                }
            }
            
            # Add launch bounds to performance info if available
            if orig_kernel.get("launch_bounds"):
                kernel_info["performance"] = {
                    "max_threads_per_block": orig_kernel["launch_bounds"]["max_threads_per_block"],
                    "min_blocks_per_multiprocessor": orig_kernel["launch_bounds"].get("min_blocks_per_multiprocessor"),
                    "notes": "Launch bounds specified via __launch_bounds__ attribute"
                }
            
            kernels.append(kernel_info)
        
        return kernels
    
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
            return f"CUDA kernel function {kernel_name}"

class CUDACompiler:
    """CUDA kernel compiler wrapper with preprocessor support."""
    
    def __init__(self):
        self.nvcc_path = self._find_nvcc()
        if not self.nvcc_path:
            raise RuntimeError("nvcc compiler not found. Please install CUDA Toolkit.")
        
        # Get standard include paths
        self.include_paths = self._get_standard_includes()
        
        # Detect available GPU architectures
        self.available_archs = self._detect_gpu_architectures()
    
    def _get_standard_includes(self) -> List[str]:
        """Get standard CUDA include paths."""
        cuda_home = os.environ.get('CUDA_HOME') or os.environ.get('CUDA_PATH') or '/usr/local/cuda'
        
        standard_paths = [
            os.path.join(cuda_home, 'include'),
            '/usr/include/cuda',
            '/usr/local/include/cuda'
        ]
        
        # Filter to existing paths
        existing_paths = [path for path in standard_paths if os.path.exists(path)]
        
        return existing_paths
    
    def _detect_gpu_architectures(self) -> List[str]:
        """Detect available GPU architectures on the system."""
        try:
            # Try to detect GPU compute capabilities using nvidia-ml-py or deviceQuery
            result = subprocess.run(['nvidia-smi', '--query-gpu=compute_cap', '--format=csv,noheader,nounits'], 
                                  capture_output=True, text=True)
            
            if result.returncode == 0:
                compute_caps = []
                for line in result.stdout.strip().split('\n'):
                    cap = line.strip().replace('.', '')
                    if cap and cap.isdigit():
                        compute_caps.append(f"sm_{cap}")
                
                return list(set(compute_caps)) if compute_caps else ["sm_70", "sm_75", "sm_80"]
            
        except:
            pass
        
        # Default architectures
        return ["sm_60", "sm_70", "sm_75", "sm_80", "sm_86"]
    
    def _find_nvcc(self) -> Optional[str]:
        """Find nvcc compiler in system PATH."""
        try:
            result = subprocess.run(['which', 'nvcc'], capture_output=True, text=True)
            if result.returncode == 0:
                return result.stdout.strip()
        except:
            pass
        
        # Try common installation paths
        cuda_paths = [
            os.environ.get('CUDA_HOME'),
            os.environ.get('CUDA_PATH'),
            '/usr/local/cuda',
            '/opt/cuda',
            '/usr/cuda'
        ]
        
        for cuda_path in cuda_paths:
            if cuda_path:
                nvcc_path = os.path.join(cuda_path, 'bin', 'nvcc')
                if os.path.isfile(nvcc_path) and os.access(nvcc_path, os.X_OK):
                    return nvcc_path
        
        return None
    
    def compile_to_binary(self, source_path: str, output_path: str, 
                         target_arch: str = "sm_70", optimization: str = "O2") -> bool:
        """Compile CUDA source to binary."""
        try:
            cmd = [
                self.nvcc_path,
                f"-{optimization}",
                f"-arch={target_arch}",
                "--shared",
                "--compiler-options", "-fPIC",
            ]
            
            # Add standard include paths
            for include_path in self.include_paths:
                cmd.extend(["-I", include_path])
            
            # Add output and source
            cmd.extend(["-o", output_path, source_path])
            
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            if result.returncode != 0:
                print(f"Compilation failed: {result.stderr}")
                return False
            
            return True
            
        except Exception as e:
            print(f"Compilation error: {e}")
            return False
    
    def compile_to_ptx(self, source_path: str, output_path: str, 
                       target_arch: str = "sm_70") -> bool:
        """Compile CUDA source to PTX intermediate representation."""
        try:
            cmd = [
                self.nvcc_path,
                f"-arch={target_arch}",
                "--ptx",
            ]
            
            # Add standard include paths
            for include_path in self.include_paths:
                cmd.extend(["-I", include_path])
            
            cmd.extend(["-o", output_path, source_path])
            
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            if result.returncode != 0:
                print(f"PTX compilation failed: {result.stderr}")
                return False
            
            return True
            
        except Exception as e:
            print(f"PTX compilation error: {e}")
            return False
    
    def preprocess_source(self, source_path: str, output_path: str = None) -> Optional[str]:
        """Preprocess source file and return preprocessed content."""
        try:
            cmd = [
                self.nvcc_path,
                "-E",  # Preprocess only
                "-D__CUDACC__",
                "-D__NVCC__",
            ]
            
            # Add standard include paths
            for include_path in self.include_paths:
                cmd.extend(["-I", include_path])
            
            cmd.append(source_path)
            
            if output_path:
                cmd.extend(["-o", output_path])
            
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
    
    def get_compiler_info(self) -> Dict:
        """Get compiler version and build information."""
        try:
            result = subprocess.run([self.nvcc_path, '--version'], 
                                  capture_output=True, text=True)
            version_info = result.stdout
            
            return {
                "compiler": "nvcc",
                "compiler_version": self._extract_version(version_info),
                "build_flags": ["--shared", "--compiler-options", "-fPIC"],
                "optimization_level": "O2",
                "debug_symbols": False,
                "available_architectures": self.available_archs
            }
        except:
            return {
                "compiler": "nvcc",
                "compiler_version": "unknown",
                "available_architectures": self.available_archs
            }
    
    def _extract_version(self, version_text: str) -> str:
        """Extract version number from compiler output."""
        # Look for version pattern in nvcc output
        match = re.search(r'release (\d+\.\d+)', version_text)
        if match:
            return match.group(1)
        
        match = re.search(r'V(\d+\.\d+\.\d+)', version_text)
        return match.group(1) if match else "unknown"

class CatalogBuilder:
    """Main catalog builder class."""
    
    def __init__(self):
        self.compiler = CUDACompiler()
    
    def build_catalog_entry(self, source_path: str, library_name: str = None,
                          target_archs: List[str] = None, include_ptx: bool = True) -> Dict:
        """Build a complete catalog entry from CUDA source."""
        
        if target_archs is None:
            target_archs = self.compiler.available_archs[:3]  # Use first 3 available
        
        source_file = Path(source_path)
        if library_name is None:
            library_name = source_file.stem
        
        # Parse kernels with preprocessor support
        parser = CUDAKernelParser(source_path, self.compiler.nvcc_path)
        kernels = parser.extract_kernels()
        
        # Build architectures
        architectures = []
        
        for arch in target_archs:
            # Compile to shared library
            with tempfile.NamedTemporaryFile(suffix=".so", delete=False) as temp_binary:
                temp_binary_path = temp_binary.name
            
            try:
                # Compile for this architecture
                success = self.compiler.compile_to_binary(
                    source_path, temp_binary_path, arch
                )
                
                if success and os.path.exists(temp_binary_path):
                    # Read binary data
                    with open(temp_binary_path, 'rb') as f:
                        binary_data = f.read()
                    
                    # Calculate checksum
                    sha256_hash = hashlib.sha256(binary_data).hexdigest()
                    
                    # Encode to base64
                    encoded_binary = base64.b64encode(binary_data).decode('utf-8')
                    
                    arch_entry = {
                        "name": "x86_64",  # Host architecture
                        "platforms": [self._detect_platform()],
                        "binary_format": "so",
                        "binary_data": encoded_binary,
                        "file_size": len(binary_data),
                        "checksum": {
                            "algorithm": "sha256",
                            "value": sha256_hash
                        },
                        "target_gpu_arch": arch
                    }
                    
                    # Add PTX if requested
                    if include_ptx:
                        ptx_data = self._compile_to_ptx(source_path, arch)
                        if ptx_data:
                            arch_entry["ptx_code"] = ptx_data
                    
                    architectures.append(arch_entry)
                
            finally:
                # Clean up temporary file
                if os.path.exists(temp_binary_path):
                    os.unlink(temp_binary_path)
        
        # Build complete library entry
        library_entry = {
            "id": f"cuda-{library_name.lower()}",
            "name": library_name,
            "version": "1.0.0",
            "description": f"CUDA kernel library: {library_name}",
            "vendor": "Custom",
            "license": "MIT",
            "categories": ["compute", "graphics"],
            "architectures": architectures,
            "functions": kernels,
            "dependencies": [
                {
                    "name": "CUDA Runtime",
                    "version": ">=10.0",
                    "description": "CUDA runtime library"
                },
                {
                    "name": "CUDA Driver",
                    "version": ">=418.0", 
                    "description": "CUDA driver"
                }
            ],
            "build_info": self.compiler.get_compiler_info(),
            "metadata": {
                "source_file": str(source_file),
                "gpu_architectures": target_archs,
                "framework": "CUDA",
                "preprocessor_used": True,
                "total_kernels": len(kernels),
                "kernel_names": [k["name"] for k in kernels],
                "includes_ptx": include_ptx
            }
        }
        
        return library_entry
    
    def _compile_to_ptx(self, source_path: str, arch: str) -> Optional[str]:
        """Compile source to PTX and return as string."""
        with tempfile.NamedTemporaryFile(suffix=".ptx", mode='w', delete=False) as temp_ptx:
            temp_ptx_path = temp_ptx.name
        
        try:
            success = self.compiler.compile_to_ptx(source_path, temp_ptx_path, arch)
            
            if success and os.path.exists(temp_ptx_path):
                with open(temp_ptx_path, 'r', encoding='utf-8') as f:
                    ptx_content = f.read()
                
                # Encode PTX as base64 for JSON storage
                return base64.b64encode(ptx_content.encode('utf-8')).decode('utf-8')
        
        except Exception as e:
            print(f"PTX compilation failed: {e}")
        
        finally:
            if os.path.exists(temp_ptx_path):
                os.unlink(temp_ptx_path)
        
        return None
    
    def _detect_platform(self) -> str:
        """Detect current platform."""
        system = platform.system().lower()
        if system == "linux":
            return "linux"
        elif system == "windows":
            return "windows"
        elif system == "darwin":
            return "macos"
        else:
            return "unknown"
    
    def build_full_catalog(self, libraries: List[Dict]) -> Dict:
        """Build complete catalog with metadata."""
        catalog = {
            "catalog": {
                "version": "1.0.0",
                "created": datetime.now().isoformat(),
                "updated": datetime.now().isoformat(),
                "description": "CUDA Kernel Library Catalog"
            },
            "libraries": libraries
        }
        
        return catalog

def main():
    parser = argparse.ArgumentParser(description="Build JSON catalog from CUDA kernel source")
    parser.add_argument("source", help="CUDA kernel source file (.cu)")
    parser.add_argument("-o", "--output", help="Output JSON file", 
                       default="cuda_catalog.json")
    parser.add_argument("-n", "--name", help="Library name", default=None)
    parser.add_argument("-a", "--archs", nargs="+", 
                       help="Target GPU architectures (e.g., sm_70, sm_80)", 
                       default=None)
    parser.add_argument("-v", "--verbose", action="store_true", 
                       help="Verbose output")
    parser.add_argument("--save-preprocessed", help="Save preprocessed source to file")
    parser.add_argument("--include", "-I", action="append", dest="include_paths",
                       help="Additional include paths", default=[])
    parser.add_argument("--no-ptx", action="store_true",
                       help="Skip PTX generation")
    
    args = parser.parse_args()
    
    if not os.path.exists(args.source):
        print(f"Error: Source file '{args.source}' not found")
        sys.exit(1)
    
    try:
        builder = CatalogBuilder()
        
        # Add additional include paths if specified
        if args.include_paths:
            builder.compiler.include_paths.extend(args.include_paths)
        
        # Use detected architectures if none specified
        target_archs = args.archs or builder.compiler.available_archs[:3]
        
        if args.verbose:
            print(f"Processing {args.source}...")
            print(f"Target architectures: {target_archs}")
            print(f"Include paths: {builder.compiler.include_paths}")
            print(f"Available GPU architectures: {builder.compiler.available_archs}")
        
        # Save preprocessed output if requested
        if args.save_preprocessed:
            if args.verbose:
                print(f"Saving preprocessed source to {args.save_preprocessed}")
            builder.compiler.preprocess_source(args.source, args.save_preprocessed)
        
        # Build library entry
        library_entry = builder.build_catalog_entry(
            args.source, args.name, target_archs, not args.no_ptx
        )
        
        # Build full catalog
        catalog = builder.build_full_catalog([library_entry])
        
        # Write to file
        with open(args.output, 'w', encoding='utf-8') as f:
            json.dump(catalog, f, indent=2)
        
        if args.verbose:
            print(f"Catalog written to {args.output}")
            print(f"Found {len(library_entry['functions'])} kernel functions:")
            for func in library_entry['functions']:
                print(f"  - {func['name']}: {len(func['parameters'])} parameters")
                if func.get('metadata', {}).get('has_preprocessed_types'):
                    print(f"    (using preprocessed types)")
                if func.get('metadata', {}).get('launch_bounds'):
                    lb = func['metadata']['launch_bounds']
                    print(f"    __launch_bounds__({lb['max_threads_per_block']}" +
                          (f", {lb['min_blocks_per_multiprocessor']}" if lb.get('min_blocks_per_multiprocessor') else "") + ")")
            
            print(f"Built for {len(library_entry['architectures'])} architectures")
            
            # Show parameter type details
            print("\nParameter Details:")
            for func in library_entry['functions']:
                print(f"\n{func['name']}:")
                for param in func['parameters']:
                    type_info = param['type']
                    if 'base_type' in param:
                        extra_info = []
                        if param.get('qualifiers'):
                            # Highlight CUDA-specific qualifiers
                            cuda_quals = [q for q in param['qualifiers'] if q.startswith('__')]
                            other_quals = [q for q in param['qualifiers'] if not q.startswith('__')]
                            if cuda_quals:
                                extra_info.append(f"CUDA qualifiers: {cuda_quals}")
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
            
            # Show PTX information if included
            ptx_count = sum(1 for arch in library_entry['architectures'] if 'ptx_code' in arch)
            if ptx_count > 0:
                print(f"\nPTX code included for {ptx_count} architecture(s)")
    
    except Exception as e:
        print(f"Error: {e}")
        if args.verbose:
            import traceback
            traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()
