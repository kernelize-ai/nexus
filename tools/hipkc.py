def main():
    parser = argparse.ArgumentParser(description="Build JSON catalog from HIP kernel source")
    parser.add_argument("source", help="HIP kernel source file (.hip)")
    parser.add_argument("-o", "--output", help="Output JSON file", 
                       default="kernel_catalog.json")
    parser.add_argument("-n", "--name", help="Library name", default=None)
    parser.add_argument("-a", "--archs", nargs="+", 
                       help="Target GPU architectures", 
                       default=["gfx906", "gfx908", "gfx90a"])
    parser.add_argument("-v", "--verbose", action="store_true", 
                       help="Verbose output")
    parser.add_argument("--save-preprocessed", help="Save preprocessed source to file")
    parser.add_argument("--include", "-I", action="append", dest="include_paths",
                       help="Additional include paths", default=[])
    
    args = parser.parse_args()
    
    if not os.path.exists(args.source):
        print(f"Error: Source file '{args.source}' not found")
        sys.exit(1)
    
    try:
        builder = CatalogBuilder()
        
        # Add additional include paths if specified
        if args.include_paths:
            builder.compiler.include_paths.extend(args.include_paths)
        
        if args.verbose:
            print(f"Processing {args.source}...")
            print(f"Target architectures: {args.archs}")
            print(f"Include paths: {builder.compiler.include_paths}")
        
        # Save preprocessed output if requested
        if args.save_preprocessed:
            if args.verbose:
                print(f"Saving preprocessed source to {args.save_preprocessed}")
            builder.compiler.preprocess_source(args.source, args.save_preprocessed)
        
        # Build library entry
        library_entry = builder.build_catalog_entry(
            args.source, args.name, args.archs
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
                            extra_info.append(f"qualifiers: {param['qualifiers']}")
                        if param.get('pointer_level', 0) > 0:
                            extra_info.append(f"pointer_level: {param['pointer_level']}")
                        if param.get('is_reference'):
                            extra_info.append("reference")
                        
                        detail = f" ({', '.join(extra_info)})" if extra_info else ""
                        print(f"    {param['name']}: {type_info}{detail}")
                    else:
                        print(f"    {param['name']}: {type_info}")
    
    except Exception as e:
        print(f"Error: {e}")
        if args.verbose:
            import traceback
            traceback.print_exc()
        sys.exit(1)#!/usr/bin/env python3
"""
HIP Kernel Catalog Builder

This tool compiles HIP kernel source files and generates a JSON catalog
with binary data encoded in Base64 format.

Requirements:
- HIP/ROCm SDK installed
- hipcc compiler available in PATH
- Python 3.6+

Usage:
    python hip_catalog_builder.py <kernel_file.hip> [options]
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

class HIPKernelParser:
    """Parser for extracting kernel information from HIP source files using preprocessor."""
    
    def __init__(self, source_path: str, compiler_path: str = None):
        self.source_path = Path(source_path)
        self.compiler_path = compiler_path or "hipcc"
        with open(source_path, 'r', encoding='utf-8') as f:
            self.source_content = f.read()
        
        # Generate preprocessed output for accurate type resolution
        self.preprocessed_content = self._preprocess_source()
    
    def _preprocess_source(self) -> str:
        """Run source through preprocessor to resolve types and macros."""
        try:
            # Create a modified version that includes debug information
            with tempfile.NamedTemporaryFile(mode='w', suffix='.hip', delete=False) as temp_source:
                # Add debug macros to capture function signatures
                debug_content = self._add_signature_capture_macros() + '\n' + self.source_content
                temp_source.write(debug_content)
                temp_source_path = temp_source.name
            
            # Run preprocessor with HIP includes
            cmd = [
                self.compiler_path,
                '-E',  # Preprocess only
                '-I/opt/rocm/include',  # Standard HIP includes
                '-I/usr/include',
                '-D__HIP_PLATFORM_AMD__',  # Define HIP platform
                '-D__HIPCC__',
                '-x', 'hip',  # Treat as HIP source
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
// Signature capture macros
#define CAPTURE_KERNEL_START(name) /* KERNEL_START: name */
#define CAPTURE_KERNEL_END(name) /* KERNEL_END: name */
#define CAPTURE_PARAM(type, name) /* PARAM: type name */

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
        
        # Enhanced regex for kernel detection
        kernel_pattern = r'__global__\s+(?:(?:__launch_bounds__\s*\([^)]+\)\s+)?(?:inline\s+)?(?:static\s+)?)?(?:void|[\w\s\*&<>:,]+)\s+(\w+)\s*\((.*?)\)\s*(?:\{|;)'
        
        for match in re.finditer(kernel_pattern, self.source_content, re.DOTALL | re.MULTILINE):
            kernel_name = match.group(1)
            params_str = match.group(2)
            
            # Get line number for better context
            line_num = self.source_content[:match.start()].count('\n') + 1
            
            kernel_info = {
                "name": kernel_name,
                "symbol": kernel_name,
                "raw_params": params_str.strip(),
                "line_number": line_num,
                "description": self._extract_description(match.start(), kernel_name),
                "return_type": "void"
            }
            
            kernels.append(kernel_info)
        
        return kernels
    
    def _extract_from_preprocessed(self) -> Dict[str, Dict]:
        """Extract type information from preprocessed content."""
        # Look for expanded function signatures in preprocessed output
        kernel_info = {}
        
        # Find function definitions in preprocessed output
        # This will have all typedefs and macros expanded
        func_pattern = r'void\s+__attribute__\s*\(\s*\(\s*global\s*\)\s*\)\s+(\w+)\s*\((.*?)\)\s*\{'
        
        for match in re.finditer(func_pattern, self.preprocessed_content, re.DOTALL):
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
        
        # More sophisticated parameter parsing for preprocessed code
        # Handle complex types, pointers, references, const qualifiers, etc.
        
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
        
        # Handle various parameter formats:
        # const float* restrict data
        # volatile int& value
        # struct MyStruct obj
        # template<class T> T* ptr
        
        # Extract qualifiers
        qualifiers = []
        qualifier_pattern = r'\b(const|volatile|restrict|__restrict__|__restrict)\b'
        for match in re.finditer(qualifier_pattern, param):
            qualifiers.append(match.group(1))
        
        # Remove qualifiers for easier parsing
        clean_param = re.sub(qualifier_pattern, '', param).strip()
        
        # Split into tokens
        tokens = clean_param.split()
        if not tokens:
            return None
        
        # Last token is usually the parameter name
        param_name = tokens[-1]
        
        # Handle pointer/reference indicators attached to name
        while param_name.startswith('*') or param_name.startswith('&'):
            param_name = param_name[1:]
        
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
        full_type_parts = qualifiers + [base_type]
        if pointer_level > 0:
            full_type_parts.append('*' * pointer_level)
        if is_reference:
            full_type_parts.append('&')
        
        full_type = ' '.join(full_type_parts)
        
        return {
            "name": param_name,
            "type": full_type,
            "base_type": base_type,
            "qualifiers": qualifiers,
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
                        "language": "cpp",
                        "code": f"// Launch {kernel_name} kernel\n{kernel_name}<<<blocks, threads>>>({', '.join(p['name'] for p in parameters)});",
                        "description": f"Basic launch of {kernel_name} kernel"
                    }
                ],
                "tags": ["hip", "gpu", "kernel", "parallel"],
                "since_version": "1.0.0",
                "metadata": {
                    "line_number": orig_kernel["line_number"],
                    "has_preprocessed_types": "expanded_params" in preprocessed_info
                }
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
            return f"HIP kernel function {kernel_name}"

class HIPCompiler:
    """HIP kernel compiler wrapper with preprocessor support."""
    
    def __init__(self):
        self.hipcc_path = self._find_hipcc()
        if not self.hipcc_path:
            raise RuntimeError("hipcc compiler not found. Please install HIP/ROCm SDK.")
        
        # Get standard include paths
        self.include_paths = self._get_standard_includes()
    
    def _get_standard_includes(self) -> List[str]:
        """Get standard HIP/ROCm include paths."""
        standard_paths = [
            "/opt/rocm/include",
            "/opt/rocm/include/hip",
            "/usr/include/hip",
            "/usr/local/include/hip"
        ]
        
        # Filter to existing paths
        existing_paths = [path for path in standard_paths if os.path.exists(path)]
        
        # Try to get from compiler
        try:
            cmd = [self.hipcc_path, "-E", "-v", "-x", "hip", "/dev/null"]
            result = subprocess.run(cmd, capture_output=True, text=True, stderr=subprocess.STDOUT)
            
            # Parse include paths from compiler output
            in_include_section = False
            for line in result.stdout.split('\n'):
                if '#include <...> search starts here:' in line:
                    in_include_section = True
                    continue
                elif 'End of search list.' in line:
                    break
                elif in_include_section and line.strip().startswith('/'):
                    path = line.strip()
                    if path not in existing_paths:
                        existing_paths.append(path)
        except:
            pass
        
        return existing_paths
    
    def _find_hipcc(self) -> Optional[str]:
        """Find hipcc compiler in system PATH."""
        try:
            result = subprocess.run(['which', 'hipcc'], capture_output=True, text=True)
            if result.returncode == 0:
                return result.stdout.strip()
        except:
            pass
        
        # Try common installation paths
        common_paths = [
            '/opt/rocm/bin/hipcc',
            '/usr/local/rocm/bin/hipcc',
            '/usr/bin/hipcc'
        ]
        
        for path in common_paths:
            if os.path.isfile(path) and os.access(path, os.X_OK):
                return path
        
        return None
    
    def compile_to_binary(self, source_path: str, output_path: str, 
                         target_arch: str = "gfx906", optimization: str = "O2") -> bool:
        """Compile HIP source to binary."""
        try:
            cmd = [
                self.hipcc_path,
                f"-{optimization}",
                f"--offload-arch={target_arch}",
                "-fPIC",
                "-shared",
            ]
            
            # Add standard include paths
            for include_path in self.include_paths:
                cmd.extend(["-I", include_path])
            
            # Add HIP-specific defines
            cmd.extend([
                "-D__HIP_PLATFORM_AMD__",
                "-D__HIPCC__",
                "-o", output_path,
                source_path
            ])
            
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            if result.returncode != 0:
                print(f"Compilation failed: {result.stderr}")
                return False
            
            return True
            
        except Exception as e:
            print(f"Compilation error: {e}")
            return False
    
    def preprocess_source(self, source_path: str, output_path: str = None) -> Optional[str]:
        """Preprocess source file and return preprocessed content."""
        try:
            cmd = [
                self.hipcc_path,
                "-E",  # Preprocess only
                "-C",  # Keep comments
            ]
            
            # Add standard include paths
            for include_path in self.include_paths:
                cmd.extend(["-I", include_path])
            
            # Add HIP-specific defines
            cmd.extend([
                "-D__HIP_PLATFORM_AMD__",
                "-D__HIPCC__",
                "-x", "hip",  # Treat as HIP source
                source_path
            ])
            
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
            result = subprocess.run([self.hipcc_path, '--version'], 
                                  capture_output=True, text=True)
            version_info = result.stdout
            
            return {
                "compiler": "hipcc",
                "compiler_version": self._extract_version(version_info),
                "build_flags": ["-fPIC", "-shared"],
                "optimization_level": "O2",
                "debug_symbols": False
            }
        except:
            return {
                "compiler": "hipcc",
                "compiler_version": "unknown"
            }
    
    def _extract_version(self, version_text: str) -> str:
        """Extract version number from compiler output."""
        # Look for version pattern
        match = re.search(r'(\d+\.\d+(?:\.\d+)?)', version_text)
        return match.group(1) if match else "unknown"

class CatalogBuilder:
    """Main catalog builder class."""
    
    def __init__(self):
        self.compiler = HIPCompiler()
    
    def build_catalog_entry(self, source_path: str, library_name: str = None,
                          target_archs: List[str] = None) -> Dict:
        """Build a complete catalog entry from HIP source."""
        
        if target_archs is None:
            target_archs = ["gfx906", "gfx908", "gfx90a"]  # Common AMD architectures
        
        source_file = Path(source_path)
        if library_name is None:
            library_name = source_file.stem
        
        # Parse kernels with preprocessor support
        parser = HIPKernelParser(source_path, self.compiler.hipcc_path)
        kernels = parser.extract_kernels()
        
        # Build architectures
        architectures = []
        
        for arch in target_archs:
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
                        "platforms": ["linux"],  # Adjust based on your platform
                        "binary_format": "so",
                        "binary_data": encoded_binary,
                        "file_size": len(binary_data),
                        "checksum": {
                            "algorithm": "sha256",
                            "value": sha256_hash
                        },
                        "target_gpu_arch": arch  # Custom field for GPU architecture
                    }
                    
                    architectures.append(arch_entry)
                
            finally:
                # Clean up temporary file
                if os.path.exists(temp_binary_path):
                    os.unlink(temp_binary_path)
        
        # Build complete library entry
        library_entry = {
            "id": f"hip-{library_name.lower()}",
            "name": library_name,
            "version": "1.0.0",
            "description": f"HIP kernel library: {library_name}",
            "vendor": "Custom",
            "license": "MIT",
            "categories": ["compute", "graphics"],
            "architectures": architectures,
            "functions": kernels,
            "dependencies": [
                {
                    "name": "HIP",
                    "version": ">=4.0.0",
                    "description": "HIP runtime library"
                },
                {
                    "name": "ROCm",
                    "version": ">=4.0.0", 
                    "description": "ROCm platform"
                }
            ],
            "build_info": self.compiler.get_compiler_info(),
            "metadata": {
                "source_file": str(source_file),
                "gpu_architectures": target_archs,
                "framework": "HIP",
                "preprocessor_used": True,
                "total_kernels": len(kernels),
                "kernel_names": [k["name"] for k in kernels]
            }
        }
        
        return library_entry
    
    def build_full_catalog(self, libraries: List[Dict]) -> Dict:
        """Build complete catalog with metadata."""
        catalog = {
            "catalog": {
                "version": "1.0.0",
                "created": datetime.now().isoformat(),
                "updated": datetime.now().isoformat(),
                "description": "HIP Kernel Library Catalog"
            },
            "libraries": libraries
        }
        
        return catalog

if __name__ == "__main__":
    main()
