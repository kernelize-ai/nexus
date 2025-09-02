#!/usr/bin/env python3
"""
CUDA Kernel Catalog Builder for CUBIN Files

This tool analyzes CUBIN binary files to extract kernel information and build
a comprehensive catalog of CUDA kernels without requiring source code.

Usage:
    python3 tools/cubin_kc.py <cubin_file> [options]
"""

import argparse
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Any


class CUBINKernelCatalogBuilder:
    """Build kernel catalog from CUDA binary files (CUBIN or shared libraries)."""
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.binary_path = None
        self.catalog = {}
        
    def extract_kernels_from_binary(self, binary_path: str) -> Dict:
        """Extract kernel information from CUDA binary file (CUBIN or shared library)."""
        self.binary_path = binary_path
        
        if not os.path.exists(binary_path):
            raise FileNotFoundError(f"Binary file not found: {binary_path}")
        
        if self.verbose:
            print(f"Processing binary file: {binary_path}")
        
        # Initialize catalog structure
        self.catalog = {
            "Format": "CUDA Kernel Catalog",
            "Version": "1.0.0",
            "Source": "CUDA Binary Analysis",
            "BinaryFile": binary_path,
            "Functions": [],
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
            "BuildInfo": self._extract_build_info(),
            "Metadata": {
                "SourceFile": binary_path,
                "AnalysisMethod": "binary_extraction",
                "TotalKernels": 0
            }
        }
        
        # Extract kernels using multiple methods
        kernels = []
        
        # Method 1: Extract from ELF symbols using nm (primary method for shared libraries)
        elf_kernels = self._extract_kernels_from_elf()
        kernels.extend(elf_kernels)
        
        # Method 2: Extract from PTX using cuobjdump (if available)
        ptx_kernels = self._extract_kernels_from_ptx()
        kernels.extend(ptx_kernels)
        
        # Method 3: Extract using nvdisasm (if available)
        disasm_kernels = self._extract_kernels_from_disassembly()
        kernels.extend(disasm_kernels)
        
        # Remove duplicates and merge information
        unique_kernels = self._merge_kernel_info(kernels)
        
        self.catalog["Functions"] = unique_kernels
        self.catalog["Metadata"]["TotalKernels"] = len(unique_kernels)
        
        if self.verbose:
            print(f"Found {len(unique_kernels)} unique kernel functions")
        
        return self.catalog
    
    def _extract_build_info(self) -> Dict:
        """Extract build information from CUBIN file."""
        build_info = {
            "Compiler": "nvcc",
            "CompilerVersion": "unknown",
            "BuildFlags": [],
            "OptimizationLevel": "unknown",
            "DebugSymbols": False,
            "TargetArchitecture": "unknown"
        }
        
        try:
            # Try to get information using cuobjdump (if available)
            result = subprocess.run(['cuobjdump', '--dump-elf', self.binary_path], 
                                  capture_output=True, text=True, timeout=30)
            if result.returncode == 0:
                # Extract architecture information
                arch_match = re.search(r'Target Architecture:\s*(\w+)', result.stdout)
                if arch_match:
                    build_info["TargetArchitecture"] = arch_match.group(1)
                
                # Check for debug symbols
                if '.debug_' in result.stdout:
                    build_info["DebugSymbols"] = True
                
        except (subprocess.TimeoutExpired, FileNotFoundError):
            pass
        
        return build_info
    
    def _extract_kernels_from_ptx(self) -> List[Dict]:
        """Extract kernel information from PTX using cuobjdump."""
        kernels = []
        
        try:
            # Extract PTX from binary (if cuobjdump is available)
            result = subprocess.run(['cuobjdump', '--dump-ptx', self.binary_path], 
                                  capture_output=True, text=True, timeout=60)
            
            if result.returncode == 0:
                ptx_content = result.stdout
                
                # Parse PTX to find kernel definitions
                kernel_pattern = r'\.entry\s+(\w+)\s*\(([^)]*)\)'
                
                for match in re.finditer(kernel_pattern, ptx_content, re.MULTILINE):
                    kernel_name = match.group(1)
                    param_str = match.group(2)
                    
                    # Skip if it's not a kernel (entry points can be functions too)
                    if not self._is_kernel_entry(ptx_content, kernel_name):
                        continue
                    
                    # Parse parameters
                    parameters = self._parse_ptx_parameters(param_str)
                    
                    # Extract additional information
                    kernel_info = self._extract_kernel_metadata_from_ptx(ptx_content, kernel_name)
                    
                    kernel_data = {
                        "Name": kernel_name,
                        "Symbol": self._mangle_symbol_from_name(kernel_name, parameters),
                        "Description": f"CUDA kernel function {kernel_name} (extracted from PTX)",
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
                        "Tags": ["cuda", "gpu", "kernel", "parallel", "binary"],
                        "SinceVersion": "1.0.0",
                        "Metadata": {
                            "ExtractionMethod": "ptx_analysis",
                            "HasSourceCode": False,
                            "LaunchBounds": kernel_info.get("LaunchBounds"),
                            "RegisterCount": kernel_info.get("RegisterCount"),
                            "SharedMemorySize": kernel_info.get("SharedMemorySize"),
                            "MaxThreadsPerBlock": kernel_info.get("MaxThreadsPerBlock")
                        }
                    }
                    
                    kernels.append(kernel_data)
                    
        except (subprocess.TimeoutExpired, FileNotFoundError) as e:
            if self.verbose:
                print(f"Warning: Could not extract PTX: {e}")
        
        return kernels
    
    def _extract_kernels_from_elf(self) -> List[Dict]:
        """Extract kernel information from ELF symbols using nm."""
        kernels = []
        
        try:
            # Check if nm is available
            nm_check = subprocess.run(['which', 'nm'], capture_output=True, text=True)
            if nm_check.returncode != 0:
                if self.verbose:
                    print("Warning: 'nm' tool not found")
                return kernels
            
            # Extract symbols using nm
            result = subprocess.run(['nm', '-D', self.binary_path], 
                                  capture_output=True, text=True, timeout=30)
            
            if result.returncode == 0:
                for line in result.stdout.split('\n'):
                    if line.strip():
                        parts = line.split()
                        if len(parts) >= 3:
                            address, symbol_type, symbol_name = parts[0], parts[1], parts[2]
                            
                            # Look for global symbols that might be kernels
                            if symbol_type in ['T', 'D', 'W'] and symbol_name.startswith('_Z'):
                                # Try to demangle the symbol
                                demangled_name = self._demangle_symbol(symbol_name)
                                if demangled_name and self._is_kernel_function(demangled_name):
                                    kernel_name = self._extract_kernel_name_from_demangled(demangled_name)
                                    parameters = self._extract_parameters_from_demangled(demangled_name)
                                    
                                    kernel_data = {
                                            "Name": kernel_name,
                                            "Symbol": symbol_name,
                                            "Description": f"CUDA kernel function {kernel_name} (extracted from ELF symbols)",
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
                                            "Tags": ["cuda", "gpu", "kernel", "parallel", "binary"],
                                            "SinceVersion": "1.0.0",
                                            "Metadata": {
                                                "ExtractionMethod": "elf_symbols",
                                                "HasSourceCode": False,
                                                "SymbolAddress": address,
                                                "SymbolType": symbol_type
                                            }
                                        }
                                        
                                    kernels.append(kernel_data)
                                    
        except (subprocess.TimeoutExpired, FileNotFoundError) as e:
            if self.verbose:
                print(f"Warning: Could not extract ELF symbols: {e}")
        
        return kernels
    
    def _extract_kernels_from_disassembly(self) -> List[Dict]:
        """Extract kernel information from disassembly using nvdisasm."""
        kernels = []
        
        try:
            # Disassemble the binary file (if nvdisasm is available)
            result = subprocess.run(['nvdisasm', self.binary_path], 
                                  capture_output=True, text=True, timeout=120)
            
            if result.returncode == 0:
                disasm_content = result.stdout
                
                # Look for kernel entry points in disassembly
                # CUDA kernels typically start with specific patterns
                kernel_pattern = r'//\s*Function:\s*(\w+)\s*\n'
                
                for match in re.finditer(kernel_pattern, disasm_content):
                    kernel_name = match.group(1)
                    
                    # Skip if it's not a kernel
                    if not self._is_kernel_in_disassembly(disasm_content, kernel_name):
                        continue
                    
                    # Extract basic information
                    kernel_info = self._extract_kernel_info_from_disassembly(disasm_content, kernel_name)
                    
                    kernel_data = {
                        "Name": kernel_name,
                        "Symbol": self._mangle_symbol_from_name(kernel_name, kernel_info.get("Parameters", [])),
                        "Description": f"CUDA kernel function {kernel_name} (extracted from disassembly)",
                        "ReturnType": "void",
                        "Parameters": kernel_info.get("Parameters", []),
                        "CallingConvention": "device",
                        "ThreadSafety": "conditionally-safe",
                        "Examples": [
                            {
                                "Language": "cuda",
                                "Code": f"// Launch {kernel_name} kernel\n{kernel_name}<<<blocks, threads>>>({', '.join(p['Name'] for p in kernel_info.get('Parameters', []))});",
                                "Description": f"Basic launch of {kernel_name} kernel"
                            }
                        ],
                        "Tags": ["cuda", "gpu", "kernel", "parallel", "binary"],
                        "SinceVersion": "1.0.0",
                        "Metadata": {
                            "ExtractionMethod": "disassembly",
                            "HasSourceCode": False,
                            "InstructionCount": kernel_info.get("InstructionCount"),
                            "RegisterCount": kernel_info.get("RegisterCount"),
                            "SharedMemorySize": kernel_info.get("SharedMemorySize")
                        }
                    }
                    
                    kernels.append(kernel_data)
                    
        except (subprocess.TimeoutExpired, FileNotFoundError) as e:
            if self.verbose:
                print(f"Warning: Could not disassemble CUBIN: {e}")
        
        return kernels
    
    def _is_kernel_entry(self, ptx_content: str, kernel_name: str) -> bool:
        """Check if an entry point is actually a kernel."""
        # Look for kernel-specific attributes in PTX
        kernel_pattern = r'\.entry\s+' + re.escape(kernel_name) + r'\s*\([^)]*\)\s*\{[^}]*\.call\s+__cudaLaunchKernel'
        return bool(re.search(kernel_pattern, ptx_content, re.DOTALL))
    
    def _is_kernel_function(self, demangled_name: str) -> bool:
        """Check if a demangled function name represents a kernel."""
        # Skip device stubs and other non-kernel functions
        if demangled_name.startswith('__device_stub__'):
            return False
        if demangled_name.startswith('test_kernels'):
            return False
        if 'dim3' in demangled_name:
            return False
        
        # CUDA kernels typically have parameters and either void return or implicit return
        has_parameters = '(' in demangled_name and ')' in demangled_name
        
        # For functions with parameters, check if they look like kernels
        if has_parameters:
            # If it has void return type, it's likely a kernel
            if 'void' in demangled_name:
                return True
            
            # If it doesn't have an explicit return type in the name, it might be a kernel
            # (CUDA kernels often have implicit void return)
            if not any(t in demangled_name.split('(')[0] for t in ['int', 'float', 'double', 'char', 'bool', 'long', 'short', 'unsigned']):
                return True
        
        return False
    
    def _is_kernel_in_disassembly(self, disasm_content: str, kernel_name: str) -> bool:
        """Check if a function in disassembly is a kernel."""
        # Look for kernel-specific patterns in disassembly
        kernel_pattern = rf'//\s*Function:\s*{re.escape(kernel_name)}.*?\n.*?\.text\s+{re.escape(kernel_name)}'
        return bool(re.search(kernel_pattern, disasm_content, re.DOTALL))
    
    def _parse_ptx_parameters(self, param_str: str) -> List[Dict]:
        """Parse PTX parameter string into structured format."""
        parameters = []
        
        if not param_str.strip():
            return parameters
        
        # PTX parameters are typically in format: .param .u64 param_name
        param_pattern = r'\.param\s+\.(\w+)\s+(\w+)'
        
        for match in re.finditer(param_pattern, param_str):
            ptx_type = match.group(1)
            param_name = match.group(2)
            
            # Convert PTX types to C++ types
            cpp_type = self._ptx_type_to_cpp_type(ptx_type)
            
            param_info = {
                "Name": param_name,
                "Type": cpp_type,
                "BaseType": self._clean_type_for_mangling(cpp_type),
                "Qualifiers": [],
                "PointerLevel": 1 if '*' in cpp_type else 0,
                "IsReference": False,
                "Description": f"Parameter {param_name} of type {cpp_type}",
                "Optional": False
            }
            
            parameters.append(param_info)
        
        return parameters
    
    def _ptx_type_to_cpp_type(self, ptx_type: str) -> str:
        """Convert PTX type to C++ type."""
        type_mapping = {
            'u8': 'unsigned char',
            's8': 'signed char',
            'u16': 'unsigned short',
            's16': 'signed short',
            'u32': 'unsigned int',
            's32': 'signed int',
            'u64': 'unsigned long long',
            's64': 'signed long long',
            'f16': 'half',
            'f32': 'float',
            'f64': 'double',
            'b8': 'bool',
            'b16': 'bool',
            'b32': 'bool',
            'b64': 'bool'
        }
        
        return type_mapping.get(ptx_type, f"unknown_{ptx_type}")
    
    def _extract_kernel_metadata_from_ptx(self, ptx_content: str, kernel_name: str) -> Dict:
        """Extract additional kernel metadata from PTX."""
        metadata = {}
        
        # Look for kernel-specific information
        kernel_section = self._find_kernel_section(ptx_content, kernel_name)
        if kernel_section:
            # Extract register count
            reg_match = re.search(r'\.reg\s+(\d+)', kernel_section)
            if reg_match:
                metadata["RegisterCount"] = int(reg_match.group(1))
            
            # Extract shared memory size
            shared_match = re.search(r'\.shared\s+(\d+)', kernel_section)
            if shared_match:
                metadata["SharedMemorySize"] = int(shared_match.group(1))
            
            # Extract launch bounds if present
            launch_bounds_match = re.search(r'\.maxntid\s+(\d+)\s*,\s*(\d+)\s*,\s*(\d+)', kernel_section)
            if launch_bounds_match:
                metadata["LaunchBounds"] = {
                    "MaxThreadsPerBlock": int(launch_bounds_match.group(1)),
                    "MaxThreadsPerBlockX": int(launch_bounds_match.group(1)),
                    "MaxThreadsPerBlockY": int(launch_bounds_match.group(2)),
                    "MaxThreadsPerBlockZ": int(launch_bounds_match.group(3))
                }
        
        return metadata
    
    def _find_kernel_section(self, ptx_content: str, kernel_name: str) -> Optional[str]:
        """Find the PTX section for a specific kernel."""
        pattern = rf'\.entry\s+{re.escape(kernel_name)}\s*\([^)]*\)\s*\{{(.*?)\}}'
        match = re.search(pattern, ptx_content, re.DOTALL)
        return match.group(1) if match else None
    
    def _demangle_symbol(self, symbol_name: str) -> Optional[str]:
        """Demangle a C++ symbol name."""
        try:
            result = subprocess.run(['c++filt', symbol_name], 
                                  capture_output=True, text=True, timeout=5)
            if result.returncode == 0:
                return result.stdout.strip()
        except (subprocess.TimeoutExpired, FileNotFoundError):
            pass
        
        return None
    
    def _extract_kernel_name_from_demangled(self, demangled_name: str) -> str:
        """Extract kernel name from demangled symbol."""
        # Extract function name from demangled string
        match = re.search(r'(\w+)\s*\(', demangled_name)
        if match:
            return match.group(1)
        
        # Fallback: extract from template instantiations
        match = re.search(r'(\w+)<[^>]*>\s*\(', demangled_name)
        if match:
            return match.group(1)
        
        return "unknown_kernel"
    
    def _extract_parameters_from_demangled(self, demangled_name: str) -> List[Dict]:
        """Extract parameter information from demangled symbol."""
        parameters = []
        
        # Extract parameter list from demangled name
        param_match = re.search(r'\(([^)]*)\)', demangled_name)
        if not param_match:
            return parameters
        
        param_str = param_match.group(1)
        if not param_str.strip():
            return parameters
        
        # Split parameters and parse each one
        param_parts = [p.strip() for p in param_str.split(',') if p.strip()]
        
        for i, param_part in enumerate(param_parts):
            # Parse parameter type and name
            type_name_match = re.search(r'([^&\*]+)([&\*]*)\s*(\w+)?', param_part)
            if type_name_match:
                param_type = type_name_match.group(1).strip()
                modifiers = type_name_match.group(2)
                param_name = type_name_match.group(3) or f"param_{i}"
                
                # Apply modifiers
                if '&' in modifiers:
                    param_type += '&'
                if '*' in modifiers:
                    param_type += '*'
                
                param_info = {
                    "Name": param_name,
                    "Type": param_type,
                    "BaseType": self._clean_type_for_mangling(param_type),
                    "Qualifiers": [],
                    "PointerLevel": param_type.count('*'),
                    "IsReference": '&' in param_type,
                    "Description": f"Parameter {param_name} of type {param_type}",
                    "Optional": False
                }
                
                parameters.append(param_info)
        
        return parameters
    
    def _extract_kernel_info_from_disassembly(self, disasm_content: str, kernel_name: str) -> Dict:
        """Extract kernel information from disassembly."""
        info = {"Parameters": []}
        
        # Find the kernel section in disassembly
        kernel_pattern = rf'//\s*Function:\s*{re.escape(kernel_name)}.*?\n(.*?)(?=//\s*Function:|$)'
        match = re.search(kernel_pattern, disasm_content, re.DOTALL)
        
        if match:
            kernel_section = match.group(1)
            
            # Count instructions
            instruction_count = len(re.findall(r'^\s*[a-zA-Z]', kernel_section, re.MULTILINE))
            info["InstructionCount"] = instruction_count
            
            # Extract register usage
            reg_match = re.search(r'\.reg\s+(\d+)', kernel_section)
            if reg_match:
                info["RegisterCount"] = int(reg_match.group(1))
            
            # Extract shared memory usage
            shared_match = re.search(r'\.shared\s+(\d+)', kernel_section)
            if shared_match:
                info["SharedMemorySize"] = int(shared_match.group(1))
        
        return info
    
    def _mangle_symbol_from_name(self, kernel_name: str, parameters: List[Dict]) -> str:
        """Generate mangled symbol name from kernel name and parameters."""
        if not parameters:
            return f"_Z{len(kernel_name)}{kernel_name}v"
        
        # Build parameter type string
        param_types = []
        for param in parameters:
            param_type = param.get('Type', 'void')
            clean_type = self._clean_type_for_mangling(param_type)
            param_types.append(clean_type)
        
        param_str = ''.join(param_types)
        return f"_Z{len(kernel_name)}{kernel_name}{param_str}"
    
    def _clean_type_for_mangling(self, type_str: str) -> str:
        """Clean type string for name mangling."""
        # Remove common qualifiers and spaces
        type_str = re.sub(r'\b(const|volatile|restrict|__restrict__|__restrict)\b', '', type_str)
        type_str = re.sub(r'\s+', '', type_str)
        
        # Handle pointer types
        if '*' in type_str:
            base_type = type_str.replace('*', '')
            clean_base = self._clean_type_for_mangling(base_type)
            return f"P{clean_base}"
        
        # Handle reference types
        if '&' in type_str:
            base_type = type_str.replace('&', '')
            clean_base = self._clean_type_for_mangling(base_type)
            return f"R{clean_base}"
        
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
        
        # Check for exact matches
        for cpp_type, mangled in type_mapping.items():
            if type_str == cpp_type:
                return mangled
        
        # Check for type with size modifiers
        for cpp_type, mangled in type_mapping.items():
            if type_str.endswith(cpp_type):
                return mangled
        
        # For unknown types, use the type name length + name
        return f"{len(type_str)}{type_str}"
    
    def _merge_kernel_info(self, kernels: List[Dict]) -> List[Dict]:
        """Merge kernel information from different extraction methods."""
        kernel_map = {}
        
        for kernel in kernels:
            kernel_name = kernel["Name"]
            
            if kernel_name not in kernel_map:
                kernel_map[kernel_name] = kernel
            else:
                # Merge information from different extraction methods
                existing = kernel_map[kernel_name]
                
                # Prefer actual symbols over calculated ones
                if kernel.get("Symbol", "").startswith("_Z") and not existing.get("Symbol", "").startswith("_Z"):
                    existing["Symbol"] = kernel["Symbol"]
                
                # Merge metadata
                existing_metadata = existing.get("Metadata", {})
                kernel_metadata = kernel.get("Metadata", {})
                
                # Combine extraction methods
                methods = existing_metadata.get("ExtractionMethods", [existing_metadata.get("ExtractionMethod", "unknown")])
                if kernel_metadata.get("ExtractionMethod") not in methods:
                    methods.append(kernel_metadata.get("ExtractionMethod", "unknown"))
                
                existing_metadata["ExtractionMethods"] = methods
                
                # Merge other metadata
                for key, value in kernel_metadata.items():
                    if key not in existing_metadata and value is not None:
                        existing_metadata[key] = value
        
        return list(kernel_map.values())
    
    def write_catalog(self, output_path: str = "cubin_catalog.json"):
        """Write the kernel catalog to a JSON file."""
        with open(output_path, 'w') as f:
            json.dump(self.catalog, f, indent=2)
        
        if self.verbose:
            print(f"Catalog written to {output_path}")


def main():
    parser = argparse.ArgumentParser(description="Build CUDA kernel catalog from CUDA binary file")
    parser.add_argument("binary_file", help="Path to the CUDA binary file to analyze (CUBIN or shared library)")
    parser.add_argument("-o", "--output", default="cuda_binary_catalog.json", 
                       help="Output JSON file path (default: cuda_binary_catalog.json)")
    parser.add_argument("-v", "--verbose", action="store_true", 
                       help="Enable verbose output")
    
    args = parser.parse_args()
    
    try:
        # Create catalog builder
        builder = CUBINKernelCatalogBuilder(verbose=args.verbose)
        
        # Extract kernels from binary
        catalog = builder.extract_kernels_from_binary(args.binary_file)
        
        # Write catalog to file
        builder.write_catalog(args.output)
        
        # Print summary
        total_kernels = len(catalog["Functions"])
        print(f"Successfully analyzed binary file: {args.binary_file}")
        print(f"Found {total_kernels} kernel functions")
        print(f"Catalog written to: {args.output}")
        
        if args.verbose and total_kernels > 0:
            print("\nKernel functions found:")
            for kernel in catalog["Functions"]:
                param_count = len(kernel.get("Parameters", []))
                symbol = kernel.get("Symbol", "unknown")
                print(f"  - {kernel['Name']}: {param_count} parameters, Symbol: {symbol}")
        
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
