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

# Import SymbolFinder for actual symbol extraction
try:
    from symbol_finder import SymbolFinder
    SYMBOL_FINDER_AVAILABLE = True
except ImportError:
    SYMBOL_FINDER_AVAILABLE = False
    print("Warning: SymbolFinder not available, using fallback symbol extraction")

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
            # Create a modified version that includes debug information and template instantiations
            with tempfile.NamedTemporaryFile(mode='w', suffix='.cu', delete=False) as temp_source:
                # Add debug macros to capture function signatures
                debug_content = self._add_signature_capture_macros() + '\n' + self.source_content
                
                # Add explicit template instantiations for template kernels
                template_instantiations = self._generate_template_instantiations()
                if template_instantiations:
                    debug_content += '\n// Explicit template instantiations\n' + template_instantiations
                
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

// Template instantiation helpers
#define INSTANTIATE_TEMPLATE(name, ...) template name<__VA_ARGS__>
#define TEMPLATE_KERNEL(name, ...) template __global__ void name<__VA_ARGS__>

// Force template instantiation for common types
#define FORCE_INSTANTIATE_TEMPLATE(name) \
    template __global__ void name<float>(); \
    template __global__ void name<double>(); \
    template __global__ void name<int>(); \
    template __global__ void name<unsigned int>();
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
                
                # Extract template information
                template_info = self._extract_template_info(match.group(0), kernel_name)
                
                kernel_info = {
                    "Name": kernel_name,
                    "Symbol": self._mangle_cuda_symbol(kernel_name),
                    "RawParams": params_str.strip(),
                    "LineNumber": line_num,
                    "Description": self._extract_description(match.start(), kernel_name),
                    "ReturnType": "void",
                    "LaunchBounds": launch_bounds,
                    "TemplateInfo": template_info
                }
                
                # Avoid duplicates, but prefer template kernels over non-template kernels
                existing_kernel = next((k for k in kernels if k["Name"] == kernel_name), None)
                if existing_kernel is None:
                    kernels.append(kernel_info)
                elif template_info.get("IsTemplate", False) and not existing_kernel.get("TemplateInfo", {}).get("IsTemplate", False):
                    # Replace non-template kernel with template kernel
                    kernels = [k for k in kernels if k["Name"] != kernel_name]
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
                "MaxThreadsPerBlock": max_threads,
                "MinBlocksPerMultiprocessor": min_blocks
            }
        
        return None
    
    def _generate_template_instantiations(self) -> str:
        """Generate explicit template instantiations for template kernels found in source."""
        instantiations = []
        
        # Find template kernels in source
        template_pattern = r'template\s*<[^>]*>\s*__global__\s+(?:void|[\w\s\*&<>:,]+)\s+(\w+)\s*\([^)]*\)'
        
        for match in re.finditer(template_pattern, self.source_content, re.DOTALL):
            kernel_name = match.group(1)
            
            # Generate instantiations for common types
            common_types = ['float', 'double', 'int', 'unsigned int', 'char', 'bool']
            
            for type_name in common_types:
                # Create explicit instantiation
                instantiation = f"template __global__ void {kernel_name}<{type_name}>();"
                instantiations.append(instantiation)
        
        return '\n'.join(instantiations)
    
    def _extract_template_info(self, kernel_signature: str, kernel_name: str) -> Dict:
        """Extract template information from kernel signature."""
        template_info = {
            "IsTemplate": False,
            "TemplateParameters": [],
            "TemplateInstantiations": [],
            "DefaultValues": {}
        }
        
        # Check if this is a template kernel
        template_match = re.search(r'template\s*<([^>]*)>', kernel_signature)
        if not template_match:
            return template_info
        
        template_info["IsTemplate"] = True
        template_params_str = template_match.group(1)
        
        # Parse template parameters
        template_params = self._parse_template_parameters(template_params_str)
        template_info["TemplateParameters"] = template_params
        
        # Look for template instantiations in the source
        instantiations = self._find_template_instantiations(kernel_name)
        template_info["TemplateInstantiations"] = instantiations
        
        return template_info
    
    def _parse_template_parameters(self, params_str: str) -> List[Dict]:
        """Parse template parameter declarations."""
        params = []
        
        # Split parameters, handling nested templates
        param_parts = self._smart_template_split(params_str)
        
        for param in param_parts:
            param = param.strip()
            if not param:
                continue
            
            # Parse different types of template parameters
            param_info = self._parse_single_template_parameter(param)
            if param_info:
                params.append(param_info)
        
        return params
    
    def _smart_template_split(self, params_str: str) -> List[str]:
        """Smart splitting of template parameters that handles nested templates."""
        params = []
        current_param = ""
        template_depth = 0
        
        for char in params_str:
            if char == ',' and template_depth == 0:
                if current_param.strip():
                    params.append(current_param.strip())
                current_param = ""
            else:
                current_param += char
                
                if char == '<':
                    template_depth += 1
                elif char == '>':
                    template_depth -= 1
        
        if current_param.strip():
            params.append(current_param.strip())
        
        return params
    
    def _parse_single_template_parameter(self, param: str) -> Optional[Dict]:
        """Parse a single template parameter declaration."""
        param = param.strip()
        if not param:
            return None
        
        # Handle typename/class parameters: typename T, class U, etc.
        typename_match = re.match(r'(typename|class)\s+(\w+)(?:\s*=\s*(.+))?', param)
        if typename_match:
            keyword, param_name, default_value = typename_match.groups()
            return {
                "Type": "typename" if keyword == "typename" else "class",
                "Name": param_name,
                "DefaultValue": default_value.strip() if default_value else None,
                "Description": f"Template type parameter {param_name}",
                "IsTypeParameter": True
            }
        
        # Handle non-type parameters: int N, size_t Size, etc.
        nontype_match = re.match(r'(\w+(?:\s*\*\s*)?)\s+(\w+)(?:\s*=\s*(.+))?', param)
        if nontype_match:
            param_type, param_name, default_value = nontype_match.groups()
            return {
                "Type": "nontype",
                "BaseType": param_type.strip(),
                "Name": param_name,
                "DefaultValue": default_value.strip() if default_value else None,
                "Description": f"Template non-type parameter {param_name} of type {param_type.strip()}",
                "IsTypeParameter": False
            }
        
        # Handle template template parameters: template<typename> class Container
        template_template_match = re.match(r'template\s*<[^>]*>\s*class\s+(\w+)(?:\s*=\s*(.+))?', param)
        if template_template_match:
            param_name, default_value = template_template_match.groups()
            return {
                "Type": "template",
                "Name": param_name,
                "DefaultValue": default_value.strip() if default_value else None,
                "Description": f"Template template parameter {param_name}",
                "IsTypeParameter": True
            }
        
        return None
    
    def _find_template_instantiations(self, kernel_name: str) -> List[Dict]:
        """Find template instantiations for a kernel in the source."""
        instantiations = []
        
        # Look for explicit template instantiations
        # Pattern: kernel_name<type1, type2, ...>
        instantiation_patterns = [
            rf'{kernel_name}\s*<([^>]+)>',  # Basic instantiation
            rf'{kernel_name}\s*<([^>]+)>\s*<<<',  # With launch configuration
            rf'{kernel_name}\s*<([^>]+)>\s*\(',   # With function call
        ]
        
        for pattern in instantiation_patterns:
            for match in re.finditer(pattern, self.source_content):
                template_args_str = match.group(1)
                template_args = self._parse_template_arguments(template_args_str)
                
                instantiation = {
                    "TemplateArguments": template_args,
                    "Location": self.source_content[:match.start()].count('\n') + 1,
                    "Context": match.group(0)
                }
                
                # Avoid duplicates
                if not any(i["TemplateArguments"] == template_args for i in instantiations):
                    instantiations.append(instantiation)
        
        return instantiations
    
    def _parse_template_arguments(self, args_str: str) -> List[Dict]:
        """Parse template arguments from instantiation."""
        args = []
        
        # Split arguments, handling nested templates
        arg_parts = self._smart_template_split(args_str)
        
        for arg in arg_parts:
            arg = arg.strip()
            if not arg:
                continue
            
            arg_info = {
                "Value": arg,
                "Type": self._infer_argument_type(arg),
                "Description": f"Template argument: {arg}"
            }
            args.append(arg_info)
        
        return args
    
    def _infer_argument_type(self, arg: str) -> str:
        """Infer the type of a template argument."""
        arg = arg.strip()
        
        # Check for common types
        if arg in ['int', 'float', 'double', 'char', 'bool', 'void']:
            return "builtin_type"
        elif arg in ['unsigned', 'signed', 'long', 'short']:
            return "type_modifier"
        elif re.match(r'^\d+$', arg):
            return "integer_literal"
        elif re.match(r'^\d+\.\d+', arg):
            return "float_literal"
        elif re.match(r'^[A-Z][A-Za-z0-9_]*$', arg):
            return "user_defined_type"
        elif '<' in arg and '>' in arg:
            return "template_type"
        elif arg.startswith('"') and arg.endswith('"'):
            return "string_literal"
        else:
            return "unknown"
    
    def _extract_from_preprocessed(self) -> Dict[str, Dict]:
        """Extract type information from preprocessed content."""
        kernel_info = {}
        
        # Find function definitions in preprocessed output
        func_patterns = [
            r'void\s+__attribute__\s*\(\s*\(\s*global\s*\)\s*\)\s+(\w+)\s*\((.*?)\)\s*\{',
            r'__global__\s+void\s+(\w+)\s*\((.*?)\)\s*\{',
            # Add patterns for template instantiations
            r'void\s+__attribute__\s*\(\s*\(\s*global\s*\)\s*\)\s+(\w+)\s*<[^>]*>\s*\((.*?)\)\s*\{',
            r'__global__\s+void\s+(\w+)\s*<[^>]*>\s*\((.*?)\)\s*\{'
        ]
        
        for pattern in func_patterns:
            for match in re.finditer(pattern, self.preprocessed_content, re.DOTALL):
                func_name = match.group(1)
                params_str = match.group(2)
                
                # Parse the fully expanded parameters
                parameters = self._parse_preprocessed_parameters(params_str)
                
                # Extract template instantiation info if present
                template_instantiation = self._extract_template_instantiation_from_preprocessed(match.group(0))
                
                # Create full instantiated name if this is a template instantiation
                full_name = func_name
                if template_instantiation:
                    template_args = template_instantiation.get("TemplateArguments", [])
                    arg_values = [arg["Value"] for arg in template_args]
                    full_name = f"{func_name}<{', '.join(arg_values)}>"
                
                kernel_info[full_name] = {
                    "ExpandedParams": parameters,
                    "FullSignature": match.group(0),
                    "TemplateInstantiation": template_instantiation
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
    
    def _extract_template_instantiation_from_preprocessed(self, signature: str) -> Optional[Dict]:
        """Extract template instantiation information from preprocessed signature."""
        # Look for template instantiation in the signature
        template_match = re.search(r'(\w+)\s*<([^>]+)>', signature)
        if not template_match:
            return None
        
        kernel_name = template_match.group(1)
        template_args_str = template_match.group(2)
        
        # Parse template arguments
        template_args = self._parse_template_arguments(template_args_str)
        
        return {
            "KernelName": kernel_name,
            "TemplateArguments": template_args,
            "InstantiatedSignature": signature
        }
    
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
            "Name": param_name,
            "Type": full_type,
            "BaseType": base_type,
            "Qualifiers": cuda_qualifiers,
            "PointerLevel": pointer_level,
            "IsReference": is_reference,
            "Description": f"Parameter {param_name} of type {full_type}",
            "Optional": False
        }
    
    def _merge_kernel_info(self, original: List[Dict], preprocessed: Dict[str, Dict]) -> List[Dict]:
        """Merge kernel information from original and preprocessed sources."""
        kernels = []
        
        for orig_kernel in original:
            kernel_name = orig_kernel["Name"]
            template_info = orig_kernel.get("TemplateInfo", {})
            
            # Handle template kernels with instantiations
            if template_info.get("IsTemplate", False):
                # Create kernel entries for each template instantiation
                instantiations = self._process_template_instantiations(
                    orig_kernel, preprocessed, template_info
                )
                kernels.extend(instantiations)
            else:
                # Handle non-template kernels
                kernel_info = self._create_kernel_info(orig_kernel, preprocessed)
                kernels.append(kernel_info)
        
        return kernels
    
    def _process_template_instantiations(self, orig_kernel: Dict, preprocessed: Dict[str, Dict], 
                                       template_info: Dict) -> List[Dict]:
        """Process template instantiations and create kernel entries for each."""
        kernels = []
        kernel_name = orig_kernel["Name"]
        
        # Get all preprocessed entries that match this kernel name (including instantiations)
        matching_preprocessed = {}
        for key, value in preprocessed.items():
            if key.startswith(kernel_name):
                matching_preprocessed[key] = value
        
        # If we have preprocessed instantiations, use them
        if matching_preprocessed:
            for instantiated_name, preprocessed_info in matching_preprocessed.items():
                kernel_info = self._create_kernel_info_from_instantiation(
                    orig_kernel, preprocessed_info, instantiated_name
                )
                kernels.append(kernel_info)
        else:
            # Fallback: create entries based on found instantiations in source
            instantiations = template_info.get("TemplateInstantiations", [])
            for instantiation in instantiations:
                kernel_info = self._create_kernel_info_from_source_instantiation(
                    orig_kernel, instantiation
                )
                kernels.append(kernel_info)
        
        return kernels
    
    def _create_kernel_info_from_instantiation(self, orig_kernel: Dict, preprocessed_info: Dict, 
                                             instantiated_name: str) -> Dict:
        """Create kernel info from preprocessed instantiation."""
        parameters = preprocessed_info.get("ExpandedParams", [])
        template_instantiation = preprocessed_info.get("TemplateInstantiation", {})
        
        # Apply template parameter substitution to ensure types are properly instantiated
        if template_instantiation and orig_kernel.get("TemplateInfo", {}).get("TemplateParameters"):
            template_args = template_instantiation.get("TemplateArguments", [])
            template_params = orig_kernel["TemplateInfo"]["TemplateParameters"]
            
            # Create substitution map
            substitution_map = {}
            for i, param in enumerate(template_params):
                if i < len(template_args):
                    substitution_map[param["Name"]] = template_args[i]["Value"]
            
            # Apply substitutions to parameters
            for param in parameters:
                param_type = param.get("Type", "")
                # Substitute template parameters in the type using word boundaries
                for template_param, arg_value in substitution_map.items():
                    # Use regex to ensure we only replace whole words, not parts of other identifiers
                    param_type = re.sub(r'\b' + re.escape(template_param) + r'\b', arg_value, param_type)
                param["Type"] = param_type
                if "BaseType" in param:
                    param["BaseType"] = self._clean_type_for_mangling(param_type)
                param["Description"] = f"Parameter {param.get('Name', '')} of type {param_type}"
        
        kernel_info = {
            "Name": instantiated_name,
            "Symbol": self._mangle_cuda_symbol(instantiated_name, parameters, template_instantiation.get("TemplateArguments")),
            "Description": f"{orig_kernel['Description']} (template instantiation)",
            "ReturnType": "void",
            "Parameters": parameters,
            "CallingConvention": "device",
            "ThreadSafety": "conditionally-safe",
            "Examples": [
                {
                    "Language": "cuda",
                    "Code": f"// Launch {instantiated_name} kernel\n{instantiated_name}<<<blocks, threads>>>({', '.join(p['Name'] for p in parameters)});",
                    "Description": f"Basic launch of {instantiated_name} kernel"
                }
            ],
            "Tags": ["cuda", "gpu", "kernel", "parallel", "template"],
            "SinceVersion": "1.0.0",
            "Metadata": {
                "LineNumber": orig_kernel["LineNumber"],
                "HasPreprocessedTypes": True,
                "LaunchBounds": orig_kernel.get("LaunchBounds"),
                "IsTemplateInstantiation": True,
                "TemplateInstantiation": template_instantiation,
                "OriginalTemplate": orig_kernel["Name"]
            }
        }
        
        # Add launch bounds to performance info if available
        if orig_kernel.get("LaunchBounds"):
            kernel_info["Performance"] = {
                "MaxThreadsPerBlock": orig_kernel["LaunchBounds"]["MaxThreadsPerBlock"],
                "MinBlocksPerMultiprocessor": orig_kernel["LaunchBounds"].get("MinBlocksPerMultiprocessor"),
                "Notes": "Launch bounds specified via __launch_bounds__ attribute"
            }
        
        return kernel_info
    
    def _create_kernel_info_from_source_instantiation(self, orig_kernel: Dict, instantiation: Dict) -> Dict:
        """Create kernel info from source-found instantiation."""
        template_args = instantiation.get("TemplateArguments", [])
        kernel_name = orig_kernel["Name"]
        
        # Create instantiated name
        arg_values = [arg["Value"] for arg in template_args]
        instantiated_name = f"{kernel_name}<{', '.join(arg_values)}>"
        
        # Use original parameters but substitute template parameters
        parameters = self._substitute_template_parameters(
            orig_kernel["RawParams"], 
            orig_kernel.get("TemplateInfo", {}).get("TemplateParameters", []),
            template_args
        )
        
        kernel_info = {
            "Name": instantiated_name,
            "Symbol": self._mangle_cuda_symbol(instantiated_name, parameters, template_args),
            "Description": f"{orig_kernel['Description']} (template instantiation)",
            "ReturnType": "void",
            "Parameters": parameters,
            "CallingConvention": "device",
            "ThreadSafety": "conditionally-safe",
            "Examples": [
                {
                    "Language": "cuda",
                    "Code": f"// Launch {instantiated_name} kernel\n{instantiated_name}<<<blocks, threads>>>({', '.join(p['Name'] for p in parameters)});",
                    "Description": f"Basic launch of {instantiated_name} kernel"
                }
            ],
            "Tags": ["cuda", "gpu", "kernel", "parallel", "template"],
            "SinceVersion": "1.0.0",
            "Metadata": {
                "LineNumber": orig_kernel["LineNumber"],
                "HasPreprocessedTypes": False,
                "LaunchBounds": orig_kernel.get("LaunchBounds"),
                "IsTemplateInstantiation": True,
                "TemplateArguments": template_args,
                "OriginalTemplate": kernel_name
            }
        }
        
        # Add launch bounds to performance info if available
        if orig_kernel.get("LaunchBounds"):
            kernel_info["Performance"] = {
                "MaxThreadsPerBlock": orig_kernel["LaunchBounds"]["MaxThreadsPerBlock"],
                "MinBlocksPerMultiprocessor": orig_kernel["LaunchBounds"].get("MinBlocksPerMultiprocessor"),
                "Notes": "Launch bounds specified via __launch_bounds__ attribute"
            }
        
        return kernel_info
    
    def _substitute_template_parameters(self, raw_params: str, template_params: List[Dict], 
                                      template_args: List[Dict]) -> List[Dict]:
        """Substitute template parameters with actual arguments in parameter types."""
        # Create substitution map
        substitution_map = {}
        for i, param in enumerate(template_params):
            if i < len(template_args):
                substitution_map[param["Name"]] = template_args[i]["Value"]
        
        # Parse parameters and substitute template parameters
        parameters = self._parse_parameters(raw_params)
        
        for param in parameters:
            original_type = param["Type"]
            param_type = original_type
            
            # Substitute template parameters in the type using word boundaries
            for template_param, arg_value in substitution_map.items():
                # Find the template parameter info to determine how to substitute
                template_param_info = next((p for p in template_params if p["Name"] == template_param), None)
                
                if template_param_info and template_param_info.get("IsTypeParameter", True):
                    # For type parameters, replace the parameter name with the actual type
                    param_type = re.sub(r'\b' + re.escape(template_param) + r'\b', arg_value, param_type)
                else:
                    # For non-type parameters, we might need different handling
                    # For now, just do simple replacement
                    param_type = re.sub(r'\b' + re.escape(template_param) + r'\b', arg_value, param_type)
            
            param["Type"] = param_type
            param["BaseType"] = self._clean_type_for_mangling(param_type)
            param["Description"] = f"Parameter {param['Name']} of type {param_type}"
        
        return parameters
    
    def _create_kernel_info(self, orig_kernel: Dict, preprocessed: Dict[str, Dict]) -> Dict:
        """Create kernel info for non-template kernels."""
        kernel_name = orig_kernel["Name"]
        
        # Get preprocessed info if available
        preprocessed_info = preprocessed.get(kernel_name, {})
        
        # Use preprocessed parameters if available, otherwise fall back to simple parsing
        if "ExpandedParams" in preprocessed_info:
            parameters = preprocessed_info["ExpandedParams"]
        else:
            parameters = self._parse_parameters(orig_kernel["RawParams"])
        
        kernel_info = {
            "Name": kernel_name,
            "Symbol": self._mangle_cuda_symbol(kernel_name, parameters),
            "Description": orig_kernel["Description"],
            "ReturnType": "void",
            "Parameters": parameters,
            "CallingConvention": "device",
            "ThreadSafety": "conditionally-safe",
            "Examples": [
                {
                    "Language": "cuda",
                    "Code": f"// Launch {kernel_name} kernel\n{kernel_name}<<<blocks, threads>>>({', '.join(p['Name'] for p in parameters)});",
                    "Description": f"Basic launch of {kernel_name} kernel"
                }
            ],
            "Tags": ["cuda", "gpu", "kernel", "parallel"],
            "SinceVersion": "1.0.0",
            "Metadata": {
                "LineNumber": orig_kernel["LineNumber"],
                "HasPreprocessedTypes": "ExpandedParams" in preprocessed_info,
                "LaunchBounds": orig_kernel.get("LaunchBounds"),
                "TemplateInfo": orig_kernel.get("TemplateInfo", {})
            }
        }
        
        # Add launch bounds to performance info if available
        if orig_kernel.get("LaunchBounds"):
            kernel_info["Performance"] = {
                "MaxThreadsPerBlock": orig_kernel["LaunchBounds"]["MaxThreadsPerBlock"],
                "MinBlocksPerMultiprocessor": orig_kernel["LaunchBounds"].get("MinBlocksPerMultiprocessor"),
                "Notes": "Launch bounds specified via __launch_bounds__ attribute"
            }
        
        return kernel_info
    
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
    
    def _mangle_cuda_symbol(self, kernel_name: str, parameters: List[Dict] = None, 
                           template_args: List[Dict] = None) -> str:
        """Generate mangled symbol name for CUDA kernel.
        
        CUDA uses a specific name mangling scheme based on the Itanium C++ ABI:
        - For non-template kernels: _Z + length + name + parameter types
        - For template kernels: _Z + length + name + template params + parameter types
        
        Note: This is a fallback implementation. The actual mangled symbols are extracted
        from the compiled binary using the 'nm' tool when available.
        """
        if not parameters:
            # Simple mangling for basic case - no parameters
            return f"_Z{len(kernel_name)}{kernel_name}v"
        
        # Build parameter type string
        param_types = []
        for param in parameters:
            param_type = param.get('Type', 'void')
            # Clean up the type for mangling
            clean_type = self._clean_type_for_mangling(param_type)
            param_types.append(clean_type)
        
        # Handle template instantiations
        if template_args:
            # Extract base kernel name (remove template arguments)
            base_name = kernel_name.split('<')[0]
            
            # Build template argument string
            template_arg_types = []
            for arg in template_args:
                arg_value = arg.get('Value', '')
                arg_type = arg.get('Type', 'unknown')
                
                # Mangle template argument based on its type
                if arg_type == 'builtin_type':
                    mangled_arg = self._clean_type_for_mangling(arg_value)
                elif arg_type == 'integer_literal':
                    mangled_arg = f"Li{arg_value}E"  # Integer literal
                elif arg_type == 'float_literal':
                    mangled_arg = f"Lf{arg_value}E"  # Float literal
                elif arg_type == 'string_literal':
                    mangled_arg = f"LA{len(arg_value)}{arg_value}E"  # String literal
                else:
                    # For user-defined types, use the type name
                    mangled_arg = f"N{len(arg_value)}{arg_value}E"
                
                template_arg_types.append(mangled_arg)
            
            # Create template instantiation mangled name
            template_str = ''.join(template_arg_types)
            param_str = ''.join(param_types)
            return f"_Z{len(base_name)}{base_name}I{template_str}E{param_str}"
        else:
            # Create mangled name with parameter types (non-template)
            param_str = ''.join(param_types)
            return f"_Z{len(kernel_name)}{kernel_name}{param_str}"
    
    def _clean_type_for_mangling(self, type_str: str) -> str:
        """Clean type string for CUDA name mangling."""
        # Remove common CUDA qualifiers and spaces
        type_str = re.sub(r'\b(const|volatile|restrict|__restrict__|__restrict)\b', '', type_str)
        type_str = re.sub(r'\b(__shared__|__constant__|__device__|__host__|__global__|__managed__)\b', '', type_str)
        type_str = re.sub(r'\s+', '', type_str)

        # Handle pointer types
        if '*' in type_str:
            base_type = type_str.replace('*', '')
            pointer_count = type_str.count('*')
            clean_base = self._clean_type_for_mangling(base_type)
            return f"P{clean_base}"
        
        # Handle reference types
        if '&' in type_str:
            base_type = type_str.replace('&', '')
            clean_base = self._clean_type_for_mangling(base_type)
            return f"R{clean_base}"
        
        # Handle array types
        if '[' in type_str and ']' in type_str:
            base_type = type_str.split('[')[0]
            clean_base = self._clean_type_for_mangling(base_type)
            return f"A{clean_base}"
        
        # Handle basic types
        type_mapping = {
            'void': 'v',
            'char': 'c',
            'short': 's',
            'int': 'i',
            'long': 'l',
            'float': 'f',
            'double': 'd',
            'bool': 'b',
            'unsigned': 'j',
            'signed': 'i',
            'unsignedchar': 'h',
            'unsignedshort': 't',
            'unsignedint': 'j',
            'unsignedlong': 'm',
            'signedchar': 'a',
            'signedshort': 's',
            'signedint': 'i',
            'signedlong': 'l',
            'longlong': 'x',
            'unsignedlonglong': 'y',
            'signedlonglong': 'x'
        }
        
        # Check for exact matches first
        for cpp_type, mangled in type_mapping.items():
            if type_str == cpp_type:
                return mangled
        
        # Check for type with size modifiers
        for cpp_type, mangled in type_mapping.items():
            if type_str.endswith(cpp_type):
                return mangled
        
        # Handle CUDA-specific types
        cuda_type_mapping = {
            'dim3': 'N3dim3E',
            'cudaStream_t': 'Pv',
            'cudaEvent_t': 'Pv',
            'cudaError_t': 'i',
            'cudaMemcpyKind': 'i',
            'cudaDeviceProp': 'N13cudaDevicePropE'
        }
        
        for cuda_type, mangled in cuda_type_mapping.items():
            if type_str == cuda_type:
                return mangled
        
        # For unknown types, use the type name length + name
        return f"{len(type_str)}{type_str}"
    
    def _extract_symbols_from_binary(self, binary_path: str) -> Dict[str, str]:
        """Extract actual mangled symbols from compiled binary using SymbolFinder."""
        symbols = {}
        
        if not SYMBOL_FINDER_AVAILABLE:
            # Fallback to basic nm extraction
            return self._extract_symbols_fallback(binary_path)
        
        try:
            # Use SymbolFinder for comprehensive symbol extraction
            finder = SymbolFinder(binary_path)
            all_symbols = finder.get_symbols()
            
            # Extract CUDA kernel symbols
            for symbol in all_symbols:
                if symbol['type'] in ['T', 't', 'W', 'w']:  # Text section symbols (functions)
                    demangled_name = symbol.get('name', finder.demangle_symbol(symbol['mangled_name']))
                    mangled_name = symbol['mangled_name']
                    
                    # Look for CUDA kernel patterns in demangled name
                    if self._is_cuda_kernel_symbol(demangled_name, mangled_name):
                        kernel_name = self._extract_kernel_name_from_symbol(demangled_name, mangled_name)
                        if kernel_name:
                            symbols[kernel_name] = mangled_name
                            print(f"Found kernel symbol: {kernel_name} -> {mangled_name}")
            
            print(f"Extracted {len(symbols)} kernel symbols from binary")
            
        except Exception as e:
            print(f"Warning: Could not extract symbols using SymbolFinder: {e}")
            # Fallback to basic extraction
            return self._extract_symbols_fallback(binary_path)
        
        return symbols
    
    def _extract_symbols_fallback(self, binary_path: str) -> Dict[str, str]:
        """Fallback symbol extraction using basic nm command."""
        symbols = {}
        try:
            # Check if nm is available
            nm_check = subprocess.run(['which', 'nm'], capture_output=True, text=True)
            if nm_check.returncode != 0:
                print("Warning: 'nm' tool not found, using calculated symbols")
                return symbols
            
            # Use nm to extract symbols
            result = subprocess.run(['nm', '-D', binary_path], capture_output=True, text=True)
            if result.returncode == 0:
                for line in result.stdout.split('\n'):
                    if line.strip():
                        parts = line.split()
                        if len(parts) >= 3:
                            address, symbol_type, symbol_name = parts[0], parts[1], parts[2]
                            # Look for global symbols (T or D)
                            if symbol_type in ['T', 'D'] and symbol_name.startswith('_Z'):
                                # Try to extract the demangled name
                                try:
                                    demangled_result = subprocess.run(['c++filt', symbol_name], 
                                                                    capture_output=True, text=True, timeout=5)
                                    if demangled_result.returncode == 0:
                                        demangled_name = demangled_result.stdout.strip()
                                        # Extract kernel name from demangled name
                                        kernel_match = re.search(r'(\w+)\s*\(', demangled_name)
                                        if kernel_match:
                                            kernel_name = kernel_match.group(1)
                                            symbols[kernel_name] = symbol_name
                                except (subprocess.TimeoutExpired, FileNotFoundError):
                                    # If c++filt is not available, try to extract kernel name from mangled symbol
                                    # This is a fallback for basic symbol extraction
                                    kernel_match = re.search(r'_Z(\d+)(\w+)', symbol_name)
                                    if kernel_match:
                                        length_str, kernel_name = kernel_match.groups()
                                        if kernel_name and kernel_name.isalpha():
                                            symbols[kernel_name] = symbol_name
        except Exception as e:
            print(f"Warning: Could not extract symbols from binary: {e}")
        
        return symbols
    
    def _is_cuda_kernel_symbol(self, demangled_name: str, mangled_name: str) -> bool:
        """Check if a symbol represents a CUDA kernel."""
        # Look for CUDA kernel patterns
        cuda_patterns = [
            r'__global__',  # CUDA kernel qualifier
            r'void\s+\w+(?:<[^>]*>)?\s*\(',  # void function with optional template parameters
            r'template.*__global__',  # Template CUDA kernel
        ]
        
        # Check demangled name
        for pattern in cuda_patterns:
            if re.search(pattern, demangled_name, re.IGNORECASE):
                return True
        
        # Check mangled name for CUDA patterns
        if mangled_name.startswith('_Z'):
            # Look for common CUDA kernel patterns in mangled names
            # CUDA kernels often have specific patterns in their mangled names
            if any(pattern in mangled_name.lower() for pattern in ['kernel', 'global', 'cuda']):
                return True
            
            # Check for template instantiation patterns
            if 'I' in mangled_name and 'E' in mangled_name:
                # This might be a template instantiation
                return True
        
        return False
    
    def _extract_kernel_name_from_symbol(self, demangled_name: str, mangled_name: str) -> Optional[str]:
        """Extract kernel name from demangled symbol name."""
        # Try to extract kernel name from demangled name
        patterns = [
            r'void\s+(\w+(?:<[^>]*>)?)\s*\(',  # void kernel_name<...>(...)
            r'(\w+(?:<[^>]*>)?)\s*\(',  # kernel_name<...>(...)
        ]
        
        for pattern in patterns:
            match = re.search(pattern, demangled_name)
            if match:
                kernel_name = match.group(1)
                # Filter out common non-kernel names
                if kernel_name not in ['main', 'malloc', 'free', 'printf', 'scanf']:
                    return kernel_name
        
        # Fallback: try to extract from mangled name
        mangled_match = re.search(r'_Z(\d+)(\w+)', mangled_name)
        if mangled_match:
            length_str, kernel_name = mangled_match.groups()
            if kernel_name and kernel_name.isalpha() and len(kernel_name) > 2:
                return kernel_name
        
        return None
    
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
                "Compiler": "nvcc",
                "CompilerVersion": self._extract_version(version_info),
                "BuildFlags": ["--shared", "--compiler-options", "-fPIC"],
                "OptimizationLevel": "O2",
                "DebugSymbols": False,
                "AvailableArchitectures": self.available_archs
            }
        except:
            return {
                "Compiler": "nvcc",
                "CompilerVersion": "unknown",
                "AvailableArchitectures": self.available_archs
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
        actual_symbols = {}  # Will store actual mangled symbols from binary
        
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
                    # Add PTX if requested
                    ptx_data = self._compile_to_ptx(source_path, arch)
                    if include_ptx and ptx_data:
                        binary_data = ptx_data
                        encoded_binary = binary_data
                        checksum_algo = "none"
                        checksum_hash = "0000000000000000000000000000000000000000000000000000000000000000"
                        binary_format = "ptx"
                    else:
                        # Read binary data
                        with open(temp_binary_path, 'rb') as f:
                            binary_data = f.read()
                        # Calculate checksum
                        checksum_algo = "sha256"
                        checksum_hash = hashlib.sha256(binary_data).hexdigest()

                        # Encode to base64
                        encoded_binary = base64.b64encode(binary_data).decode('utf-8')
                        binary_format = "so"                    

                    
                    # Extract actual mangled symbols from binary
                    arch_symbols = parser._extract_symbols_from_binary(temp_binary_path)
                    actual_symbols.update(arch_symbols)  # Merge symbols from all architectures

                    arch_entry = {
                        "Name": arch,  # Use GPU architecture as the Name
                        "Platforms": [self._detect_platform()],
                        "BinaryFormat": binary_format,
                        "BinaryData": encoded_binary,
                        "FileSize": len(binary_data),
                        "Checksum": {
                            "Algorithm": checksum_algo,
                            "Value": checksum_hash
                        },
                        "Symbols": arch_symbols  # Store actual mangled symbols for this arch
                    }
                    
                    architectures.append(arch_entry)
                
            finally:
                # Clean up temporary file
                if os.path.exists(temp_binary_path):
                    os.unlink(temp_binary_path)
        
        # Update kernel symbols with actual mangled symbols if available
        self._update_kernel_symbols(kernels, actual_symbols)
        
        # Build complete library entry
        library_entry = {
            "Id": f"cuda-{library_name.lower()}",
            "Name": library_name,
            "Version": "1.0.0",
            "Description": f"CUDA kernel library: {library_name}",
            "Vendor": "Custom",
            "License": "MIT",
            "Categories": ["compute", "graphics"],
            "Architectures": architectures,
            "Functions": kernels,
            "Dependencies": [
                {
                    "Name": "CUDA Runtime",
                    "Version": ">=10.0",
                    "Description": "CUDA runtime library"
                },
                {
                    "Name": "CUDA Driver",
                    "Version": ">=418.0", 
                    "Description": "CUDA driver"
                }
            ],
            "BuildInfo": self.compiler.get_compiler_info(),
            "Metadata": {
                "SourceFile": str(source_file),
                "GpuArchitectures": target_archs,
                "Framework": "CUDA",
                "PreprocessorUsed": True,
                "TotalKernels": len(kernels),
                "KernelNames": [k["Name"] for k in kernels],
                "IncludesPtx": include_ptx,
                "SymbolsExtracted": len(actual_symbols) > 0,
                "SymbolExtractionMethod": "SymbolFinder" if SYMBOL_FINDER_AVAILABLE else "nm_fallback",
                "ActualSymbolsFound": len(actual_symbols)
            }
        }
        
        return library_entry
    
    def _update_kernel_symbols(self, kernels: List[Dict], actual_symbols: Dict[str, str]):
        """Update kernel information with actual mangled symbols from binary."""
        for kernel in kernels:
            kernel_name = kernel["Name"]
            
            # Initialize metadata if not present
            if "Metadata" not in kernel:
                kernel["Metadata"] = {}
            
            # Check for exact match first
            if kernel_name in actual_symbols:
                # Update with actual mangled symbol
                kernel["Symbol"] = actual_symbols[kernel_name]
                kernel["Metadata"]["ActualSymbol"] = True
                kernel["Metadata"]["SymbolSource"] = "binary_extraction"
                print(f"Updated kernel '{kernel_name}' with actual symbol: {actual_symbols[kernel_name]}")
            else:
                # Try to find template instantiations for template kernels
                found_symbol = False
                if kernel.get("TemplateInfo", {}).get("IsTemplate", False):
                    found_symbol = self._find_template_instantiations(kernel, actual_symbols)
                
                if not found_symbol:
                    # Keep calculated symbol but mark as not verified
                    kernel["Metadata"]["ActualSymbol"] = False
                    kernel["Metadata"]["SymbolSource"] = "calculated"
                    print(f"Kernel '{kernel_name}' not found in binary, using calculated symbol: {kernel.get('Symbol', 'N/A')}")
    
    def _find_template_instantiations(self, kernel: Dict, actual_symbols: Dict[str, str]) -> bool:
        """Find template instantiations for a kernel in the actual symbols."""
        kernel_name = kernel["Name"]
        template_instantiations = []
        found_symbol = False
        
        # Look for template instantiations in actual symbols
        for symbol_name, mangled_symbol in actual_symbols.items():
            # Check if this symbol matches our template kernel
            if self._is_template_instantiation_match(kernel_name, symbol_name):
                template_instantiations.append({
                    "DemangledName": symbol_name,
                    "MangledSymbol": mangled_symbol
                })
                found_symbol = True
        
        if template_instantiations:
            kernel["Metadata"]["FoundTemplateInstantiations"] = template_instantiations
            print(f"Found {len(template_instantiations)} template instantiations for kernel '{kernel_name}'")
            
            # Use the first template instantiation as the primary symbol
            if template_instantiations:
                primary_instantiation = template_instantiations[0]
                kernel["Symbol"] = primary_instantiation["MangledSymbol"]
                kernel["Metadata"]["ActualSymbol"] = True
                kernel["Metadata"]["SymbolSource"] = "template_instantiation"
                print(f"Updated kernel '{kernel_name}' with template instantiation: {primary_instantiation['MangledSymbol']}")
        
        return found_symbol
    
    def _is_template_instantiation_match(self, kernel_name: str, symbol_name: str) -> bool:
        """Check if a symbol name matches a template kernel instantiation."""
        # Extract base kernel name (without template parameters)
        base_kernel_name = kernel_name.split('<')[0]
        
        # Check if the symbol name contains the base kernel name
        if base_kernel_name in symbol_name:
            # Additional checks to ensure it's a template instantiation
            # Look for template parameter patterns in the symbol name
            if '<' in symbol_name and '>' in symbol_name:
                return True
            
            # Check for mangled template patterns
            if '_Z' in symbol_name and any(char.isdigit() for char in symbol_name):
                return True
        
        return False
    
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
                return ptx_content
        
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
            "Catalog": {
                "Version": "1.0.0",
                "Created": datetime.now().isoformat(),
                "Updated": datetime.now().isoformat(),
                "Description": "CUDA Kernel Library Catalog"
            },
            "Libraries": libraries
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
            print(f"Found {len(library_entry['Functions'])} kernel functions:")
            for func in library_entry['Functions']:
                print(f"  - {func['Name']}: {len(func['Parameters'])} parameters")
                if func.get('Metadata', {}).get('HasPreprocessedTypes'):
                    print(f"    (using preprocessed types)")
                if func.get('Metadata', {}).get('IsTemplateInstantiation'):
                    print(f"    (template instantiation)")
                    if func.get('Metadata', {}).get('OriginalTemplate'):
                        print(f"    (original template: {func['Metadata']['OriginalTemplate']})")
                if func.get('Metadata', {}).get('LaunchBounds'):
                    lb = func['Metadata']['LaunchBounds']
                    print(f"    __launch_bounds__({lb['MaxThreadsPerBlock']}" +
                          (f", {lb['MinBlocksPerMultiprocessor']}" if lb.get('MinBlocksPerMultiprocessor') else "") + ")")
                if func.get('Metadata', {}).get('ActualSymbol'):
                    print(f"    Symbol: {func['Symbol']} (extracted from binary)")
                else:
                    print(f"    Symbol: {func['Symbol']} (calculated)")
            
            print(f"Built for {len(library_entry['Architectures'])} architectures")
            
            # Show parameter type details
            print("\nParameter Details:")
            for func in library_entry['Functions']:
                print(f"\n{func['Name']}:")
                for param in func['Parameters']:
                    type_info = param['Type']
                    if 'BaseType' in param:
                        extra_info = []
                        if param.get('Qualifiers'):
                            # Highlight CUDA-specific qualifiers
                            cuda_quals = [q for q in param['Qualifiers'] if q.startswith('__')]
                            other_quals = [q for q in param['Qualifiers'] if not q.startswith('__')]
                            if cuda_quals:
                                extra_info.append(f"CUDA qualifiers: {cuda_quals}")
                            if other_quals:
                                extra_info.append(f"qualifiers: {other_quals}")
                        if param.get('PointerLevel', 0) > 0:
                            extra_info.append(f"pointer_level: {param['PointerLevel']}")
                        if param.get('IsReference'):
                            extra_info.append("reference")
                        
                        detail = f" ({', '.join(extra_info)})" if extra_info else ""
                        print(f"    {param['Name']}: {type_info}{detail}")
                    else:
                        print(f"    {param['Name']}: {type_info}")
            
            # Show PTX information if included
            ptx_count = sum(1 for arch in library_entry['Architectures'] if 'PtxCode' in arch)
            if ptx_count > 0:
                print(f"\nPTX code included for {ptx_count} architecture(s)")
            
            # Show symbol extraction information
            actual_symbols_count = sum(1 for func in library_entry['Functions'] 
                                     if func.get('Metadata', {}).get('ActualSymbol'))
            if actual_symbols_count > 0:
                print(f"Actual mangled symbols extracted for {actual_symbols_count}/{len(library_entry['Functions'])} kernels")
            else:
                print("Using calculated mangled symbols (could not extract from binary)")
            
            # Show template instantiation information
            template_instantiations = [func for func in library_entry['Functions'] 
                                     if func.get('Metadata', {}).get('IsTemplateInstantiation')]
            if template_instantiations:
                print(f"\nTemplate instantiations found: {len(template_instantiations)}")
                for func in template_instantiations:
                    print(f"  - {func['Name']} (from {func['Metadata']['OriginalTemplate']})")
                    # Show parameter type substitution details
                    if func.get('Parameters'):
                        print(f"    Parameter types after template substitution:")
                        for param in func['Parameters']:
                            print(f"      {param['Name']}: {param['Type']}")
    
    except Exception as e:
        print(f"Error: {e}")
        if args.verbose:
            import traceback
            traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()
