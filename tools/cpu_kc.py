#!/usr/bin/env python3
"""
C++ Library Binary Catalog Builder

This tool analyzes compiled C++ libraries, extracts function signatures using 
preprocessor output, gets mangled symbols from binaries, and creates a JSON 
catalog with base64-encoded library data.
"""

import json
import re
import argparse
import hashlib
import base64
import subprocess
import tempfile
import shutil
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Any, Optional, Tuple, Set
import struct
import platform
import os


class CppLibraryCatalogBuilder:
    def __init__(self):
        self.platform_info = self._detect_platform()
        self.available_tools = self._check_available_tools()
        
    def _detect_platform(self) -> Dict[str, str]:
        """Detect current platform information"""
        system = platform.system()
        machine = platform.machine()
        
        # Map to schema enums
        host_arch_map = {
            'x86_64': 'X86_64', 'AMD64': 'X86_64',
            'i386': 'X86', 'i686': 'X86',
            'arm64': 'ARM64', 'aarch64': 'AArch64',
            'armv7l': 'ARM'
        }
        
        platform_map = {
            'Windows': 'Windows',
            'Linux': 'Linux', 
            'Darwin': 'MacOS'
        }
        
        binary_format_map = {
            'Windows': 'DLL',
            'Linux': 'SO',
            'Darwin': 'DYLIB'
        }
        
        return {
            'system': system,
            'host_arch': host_arch_map.get(machine, 'X86_64'),
            'platform': platform_map.get(system, 'Linux'),
            'binary_format': binary_format_map.get(system, 'SO'),
            'machine': machine
        }
    
    def _check_available_tools(self) -> Dict[str, bool]:
        """Check availability of required tools"""
        tools = {}
        
        # Check for compilers
        for compiler in ['g++', 'clang++', 'cl.exe']:
            tools[compiler] = shutil.which(compiler) is not None
            
        # Check for symbol extraction tools
        for tool in ['nm', 'objdump', 'readelf', 'dumpbin']:
            tools[tool] = shutil.which(tool) is not None
            
        # Check for ar/lib tools
        for tool in ['ar', 'lib']:
            tools[tool] = shutil.which(tool) is not None
            
        return tools

    def analyze_header_with_preprocessor(self, header_path: str, 
                                       include_dirs: List[str] = None,
                                       compiler: str = None) -> Dict[str, Any]:
        """Use preprocessor to extract accurate type information"""
        include_dirs = include_dirs or []
        compiler = compiler or self._get_default_compiler()
        
        if not self.available_tools.get(compiler, False):
            raise RuntimeError(f"Compiler {compiler} not available")
        
        # Create preprocessor command
        cmd = [compiler, '-E', '-dM']  # -E for preprocess only, -dM for macros
        
        # Add include directories
        for inc_dir in include_dirs:
            cmd.extend(['-I', inc_dir])
            
        # Add standard includes
        cmd.extend(['-I/usr/include', '-I/usr/local/include'])
        
        cmd.append(header_path)
        
        try:
            # Run preprocessor
            result = subprocess.run(cmd, capture_output=True, text=True, check=True)
            
            # Also get expanded source without macros for function parsing
            cmd_expanded = [c for c in cmd if c != '-dM']
            result_expanded = subprocess.run(cmd_expanded, capture_output=True, text=True, check=True)
            
            return {
                'macros': self._parse_macros(result.stdout),
                'expanded_source': result_expanded.stdout,
                'original_file': header_path,
                'compiler_used': compiler
            }
            
        except subprocess.CalledProcessError as e:
            raise RuntimeError(f"Preprocessor failed: {e.stderr}")

    def _get_default_compiler(self) -> str:
        """Get default compiler for platform"""
        if self.available_tools.get('g++'):
            return 'g++'
        elif self.available_tools.get('clang++'):
            return 'clang++'
        elif self.available_tools.get('cl.exe'):
            return 'cl.exe'
        else:
            raise RuntimeError("No suitable C++ compiler found")

    def _parse_macros(self, macro_output: str) -> Dict[str, str]:
        """Parse preprocessor macro definitions"""
        macros = {}
        for line in macro_output.splitlines():
            if line.startswith('#define '):
                parts = line[8:].split(' ', 1)
                if len(parts) >= 1:
                    name = parts[0]
                    value = parts[1] if len(parts) > 1 else ''
                    macros[name] = value
        return macros

    def extract_functions_from_preprocessed(self, preprocessed_data: Dict[str, Any]) -> List[Dict[str, Any]]:
        """Extract function signatures from preprocessed source"""
        expanded_source = preprocessed_data['expanded_source']
        
        # Remove line directives and clean up
        cleaned_source = self._clean_preprocessed_source(expanded_source)
        
        # Extract function declarations/definitions
        functions = []
        
        # Pattern for function declarations (more robust than before)
        # Matches: return_type function_name(parameters) [const] [noexcept] [= default/delete] [;/{]
        function_pattern = r'''
            (?:^|\n)                          # Start of line
            (?:(?:inline|static|extern|virtual|explicit)\s+)*  # Optional specifiers
            ((?:const\s+|volatile\s+|unsigned\s+|signed\s+|long\s+|short\s+)*  # Type qualifiers
             (?:\w+(?:::\w+)*(?:\s*<[^>]*>)?(?:\s*\*|\s*&)*\s+))  # Return type with namespace/template/pointer
            (\w+)                             # Function name
            \s*\(([^)]*)\)                    # Parameters in parentheses
            (?:\s*const)?                     # Optional const
            (?:\s*noexcept(?:\([^)]*\))?)?    # Optional noexcept
            (?:\s*=\s*(?:default|delete))?    # Optional = default/delete
            \s*[;{]                           # Ending with ; or {
        '''
        
        matches = re.finditer(function_pattern, cleaned_source, re.VERBOSE | re.MULTILINE)
        
        for match in matches:
            return_type = match.group(1).strip()
            func_name = match.group(2)
            params_str = match.group(3)
            
            # Skip if this looks like a macro expansion artifact
            if any(c in func_name for c in '<>()'):
                continue
                
            function_info = {
                "Name": func_name,
                "Symbol": func_name,  # Will be updated with mangled name later
                "Description": "",
                "ReturnType": self._normalize_type(return_type),
                "Parameters": self._parse_parameters_preprocessed(params_str),
                "CallingConvention": "CDECL",
                "ThreadSafety": "Unknown",
                "Deprecated": False,
                "SinceVersion": "1.0.0"
            }
            
            functions.append(function_info)
        
        return functions

    def _clean_preprocessed_source(self, source: str) -> str:
        """Clean preprocessed source for easier parsing"""
        lines = []
        for line in source.splitlines():
            # Skip line directives
            if line.startswith('#'):
                continue
            # Skip empty lines
            if not line.strip():
                continue
            lines.append(line)
        
        return '\n'.join(lines)

    def _normalize_type(self, type_str: str) -> str:
        """Normalize C++ type string"""
        # Remove extra whitespace
        normalized = ' '.join(type_str.split())
        
        # Standardize pointer/reference spacing
        normalized = re.sub(r'\s*\*\s*', '* ', normalized)
        normalized = re.sub(r'\s*&\s*', '& ', normalized)
        
        return normalized.strip()

    def _parse_parameters_preprocessed(self, params_str: str) -> List[Dict[str, Any]]:
        """Parse parameters from preprocessed function signature"""
        parameters = []
        
        if not params_str.strip() or params_str.strip() == 'void':
            return parameters
        
        # Simple parameter splitting (could be enhanced for complex templates)
        param_parts = self._smart_parameter_split(params_str)
        
        for i, param_str in enumerate(param_parts):
            param_str = param_str.strip()
            if not param_str:
                continue
            
            # Parse parameter: [type] [name] [= default_value]
            param_info = self._parse_single_parameter(param_str, i)
            if param_info:
                parameters.append(param_info)
        
        return parameters

    def _smart_parameter_split(self, params_str: str) -> List[str]:
        """Smart parameter splitting that handles templates and nested types"""
        params = []
        current_param = ""
        paren_depth = 0
        angle_depth = 0
        
        for char in params_str:
            if char == ',' and paren_depth == 0 and angle_depth == 0:
                params.append(current_param.strip())
                current_param = ""
            else:
                if char == '(':
                    paren_depth += 1
                elif char == ')':
                    paren_depth -= 1
                elif char == '<':
                    angle_depth += 1
                elif char == '>':
                    angle_depth -= 1
                current_param += char
        
        if current_param.strip():
            params.append(current_param.strip())
            
        return params

    def _parse_single_parameter(self, param_str: str, index: int) -> Optional[Dict[str, Any]]:
        """Parse a single parameter string"""
        # Handle default values
        default_value = None
        if '=' in param_str:
            param_str, default_value = param_str.split('=', 1)
            param_str = param_str.strip()
            default_value = default_value.strip()
        
        # Handle array syntax - convert [] to *
        was_array = '[]' in param_str
        if was_array:
            # Remove [] and add * to the type, not the name
            param_str = param_str.replace('[]', '')
            # We'll add the * to the type later in the parsing
        
        # Split into tokens
        tokens = param_str.split()
        if not tokens:
            return None
        
        # Last token is usually the parameter name (unless it's just a type)
        if len(tokens) == 1:
            # Just a type, no name
            param_type = tokens[0]
            param_name = f"param_{index}"
        else:
            param_name = tokens[-1]
            param_type = ' '.join(tokens[:-1])
        
        # Handle pointer/reference in name
        while param_name.startswith('*') or param_name.startswith('&'):
            if param_name.startswith('*'):
                param_type += '*'
            else:
                param_type += '&'
            param_name = param_name[1:]
        
        # If this was originally an array, add pointer to type
        if was_array:
            param_type += '*'
        
        parameter = {
            "Name": param_name,
            "Type": self._normalize_type(param_type),
            "BaseType": self._extract_base_type(param_type),
            "Description": "",
            "Optional": default_value is not None,
            "Qualifiers": self._extract_type_qualifiers(param_type),
            "PointerLevel": param_type.count('*'),
            "IsReference": '&' in param_type
        }
        
        if default_value:
            parameter["DefaultValue"] = default_value
            
        return parameter

    def _extract_base_type(self, type_str: str) -> str:
        """Extract base type without qualifiers"""
        base = type_str
        # Remove qualifiers
        for qual in ['const', 'volatile', 'static', 'extern', 'mutable']:
            base = re.sub(rf'\b{qual}\b', '', base)
        # Remove pointers and references
        base = re.sub(r'[*&]+', '', base)
        return ' '.join(base.split())

    def _extract_type_qualifiers(self, type_str: str) -> List[str]:
        """Extract type qualifiers"""
        qualifiers = []
        for qual in ['const', 'volatile', 'static', 'extern', 'mutable']:
            if re.search(rf'\b{qual}\b', type_str):
                qualifiers.append(qual)
        return qualifiers

    def analyze_binary_library(self, library_path: str) -> Dict[str, Any]:
        """Analyze compiled binary library"""
        lib_path = Path(library_path)
        if not lib_path.exists():
            raise FileNotFoundError(f"Library file not found: {library_path}")
        
        # Get file info
        file_size = lib_path.stat().st_size
        
        # Calculate checksums
        checksums = self._calculate_checksums(library_path)
        
        # Extract symbols
        symbols = self._extract_symbols(library_path)
        
        # Encode binary data
        binary_data = self._encode_binary_data(library_path)
        
        # Determine binary format
        binary_format = self._detect_binary_format(library_path)
        
        return {
            'file_path': str(lib_path),
            'file_size': file_size,
            'checksums': checksums,
            'symbols': symbols,
            'binary_data': binary_data,
            'binary_format': binary_format
        }

    def _calculate_checksums(self, file_path: str) -> Dict[str, Dict[str, str]]:
        """Calculate multiple checksums for the file"""
        checksums = {}
        
        with open(file_path, 'rb') as f:
            data = f.read()
        
        # Calculate different hash types
        hash_algorithms = {
            'MD5': hashlib.md5,
            'SHA1': hashlib.sha1,
            'SHA256': hashlib.sha256,
            'SHA512': hashlib.sha512
        }
        
        for name, hash_func in hash_algorithms.items():
            hash_obj = hash_func()
            hash_obj.update(data)
            checksums[name] = {
                'Algorithm': name,
                'Value': hash_obj.hexdigest()
            }
        
        return checksums

    def _extract_symbols(self, library_path: str) -> List[Dict[str, Any]]:
        """Extract symbol table from binary"""
        symbols = []
        
        try:
            if self.platform_info['system'] == 'Linux' and self.available_tools.get('nm'):
                symbols = self._extract_symbols_nm(library_path)
            elif self.platform_info['system'] == 'Darwin' and self.available_tools.get('nm'):
                symbols = self._extract_symbols_nm(library_path)
            elif self.platform_info['system'] == 'Windows' and self.available_tools.get('dumpbin'):
                symbols = self._extract_symbols_dumpbin(library_path)
            else:
                # Try objdump as fallback
                if self.available_tools.get('objdump'):
                    symbols = self._extract_symbols_objdump(library_path)
                    
        except Exception as e:
            print(f"Warning: Could not extract symbols: {e}")
        
        return symbols

    def _extract_symbols_nm(self, library_path: str) -> List[Dict[str, Any]]:
        """Extract symbols using nm tool"""
        cmd = ['nm', '--defined-only', '--extern-only', library_path]
        
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, check=True)
            symbols = []
            
            for line in result.stdout.splitlines():
                parts = line.strip().split()
                if len(parts) >= 3:
                    address = parts[0]
                    symbol_type = parts[1]
                    symbol_name = parts[2]
                    
                    # Filter for function symbols
                    if symbol_type.upper() in ['T', 'W']:  # Text/Weak symbols
                        symbols.append({
                            'name': symbol_name,
                            'mangled_name': symbol_name,
                            'address': address,
                            'type': 'function',
                            'demangled_name': self._demangle_symbol(symbol_name)
                        })
            
            return symbols
            
        except subprocess.CalledProcessError:
            return []

    def _extract_symbols_objdump(self, library_path: str) -> List[Dict[str, Any]]:
        """Extract symbols using objdump"""
        cmd = ['objdump', '-t', library_path]
        
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, check=True)
            symbols = []
            
            for line in result.stdout.splitlines():
                # Parse objdump symbol table format
                if 'F' in line and '.text' in line:  # Function in text section
                    parts = line.split()
                    if len(parts) >= 6:
                        symbol_name = parts[-1]
                        address = parts[0]
                        
                        symbols.append({
                            'name': symbol_name,
                            'mangled_name': symbol_name,
                            'address': address,
                            'type': 'function',
                            'demangled_name': self._demangle_symbol(symbol_name)
                        })
            
            return symbols
            
        except subprocess.CalledProcessError:
            return []

    def _extract_symbols_dumpbin(self, library_path: str) -> List[Dict[str, Any]]:
        """Extract symbols using Windows dumpbin"""
        cmd = ['dumpbin', '/SYMBOLS', library_path]
        
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, check=True)
            # Parse Windows PE symbol format
            # This would need more specific implementation for PE format
            return []
            
        except subprocess.CalledProcessError:
            return []

    def _demangle_symbol(self, mangled_name: str) -> str:
        """Attempt to demangle C++ symbol name"""
        try:
            # Try c++filt if available
            if shutil.which('c++filt'):
                result = subprocess.run(['c++filt', mangled_name], 
                                      capture_output=True, text=True)
                if result.returncode == 0:
                    return result.stdout.strip()
        except:
            pass
        
        return mangled_name  # Return original if demangling fails

    def _generate_mangled_name(self, function: Dict[str, Any]) -> str:
        """Generate mangled name for C++ function"""
        func_name = function['Name']
        params = function.get('Parameters', [])
        
        # For template functions, we need to include template arguments
        if function.get('TemplateParameters'):
            # Get resolved template arguments if available
            resolved_args = function.get('ResolvedTemplateArgs', [])
            if resolved_args:
                # Create template instantiation signature
                template_sig = f"{func_name}<{','.join(resolved_args)}>"
            else:
                # Use template parameter names
                template_params = [tp['Name'] for tp in function['TemplateParameters']]
                template_sig = f"{func_name}<{','.join(template_params)}>"
        else:
            template_sig = func_name
        
        # Build parameter signature
        param_types = []
        for param in params:
            param_type = param.get('Type', 'void')
            # Handle pointer types
            if '*' in param_type:
                param_types.append(param_type)
            else:
                param_types.append(param_type)
        
        # Create function signature
        if param_types:
            signature = f"{template_sig}({','.join(param_types)})"
        else:
            signature = f"{template_sig}()"
        
        # Improved C++ name mangling based on actual patterns
        # Format: _Z{length}{name}{template_args}{parameter_types}
        
        # Start with _Z
        mangled = "_Z"
        
        # Add function name length and name
        mangled += f"{len(func_name)}{func_name}"
        
        # Add template arguments if present
        if function.get('TemplateParameters') and function.get('ResolvedTemplateArgs'):
            template_args = function['ResolvedTemplateArgs']
            for arg in template_args:
                # Map common types to their mangled forms
                if arg == 'int':
                    mangled += "Ii"
                elif arg == 'double':
                    mangled += "Id"
                elif arg == 'float':
                    mangled += "If"
                elif arg == 'char':
                    mangled += "Ic"
                elif arg == 'bool':
                    mangled += "Ib"
                elif arg == 'long':
                    mangled += "Il"
                elif arg == 'unsigned':
                    mangled += "Ij"
                else:
                    # For other types, use a simple approach
                    mangled += f"{len(arg)}{arg}"
        
        # Add return type and parameter types
        return_type = function.get('ReturnType', 'void')
        
        # Add E before return type
        mangled += "E"
        
        # Map return type
        if return_type == 'void':
            mangled += "v"
        elif return_type == 'int':
            mangled += "i"
        elif return_type == 'double':
            mangled += "d"
        elif return_type == 'float':
            mangled += "f"
        elif return_type == 'char':
            mangled += "c"
        elif return_type == 'bool':
            mangled += "b"
        elif return_type == 'long':
            mangled += "l"
        elif return_type == 'unsigned':
            mangled += "j"
        else:
            mangled += f"{len(return_type)}{return_type}"
        
        # Add parameter types
        if param_types:
            # Track unique parameter types for substitution
            unique_types = []
            type_substitutions = {}
            substitution_count = 0
            
            for param_type in param_types:
                if param_type not in unique_types:
                    unique_types.append(param_type)
                    substitution_count += 1
                    type_substitutions[param_type] = substitution_count
            
            for i, param_type in enumerate(param_types):
                # Check if this type has been seen before (substitution)
                if param_type in type_substitutions and i > 0:
                    # Check if this is a repeat of a previous type
                    first_occurrence = param_types.index(param_type)
                    if first_occurrence < i:
                        # Use substitution reference
                        mangled += f"S{type_substitutions[param_type]}_"
                        continue
                
                # Map common types to their mangled forms
                if param_type == 'int':
                    mangled += "i"
                elif param_type == 'double':
                    mangled += "d"
                elif param_type == 'float':
                    mangled += "f"
                elif param_type == 'char':
                    mangled += "c"
                elif param_type == 'bool':
                    mangled += "b"
                elif param_type == 'long':
                    mangled += "l"
                elif param_type == 'unsigned':
                    mangled += "j"
                elif param_type == 'int64':
                    mangled += "x"  # long long
                elif '*' in param_type:
                    # Handle pointer types
                    base_type = param_type.replace('*', '')
                    if base_type == 'int':
                        mangled += "PT_"  # First occurrence of int*
                    elif base_type == 'double':
                        mangled += "Pd"
                    elif base_type == 'float':
                        mangled += "Pf"
                    elif base_type == 'char':
                        mangled += "Pc"
                    elif base_type == 'int64':
                        mangled += "Px"  # long long*
                    else:
                        mangled += f"P{len(base_type)}{base_type}"
                else:
                    # For other types, use a simple approach
                    mangled += f"{len(param_type)}{param_type}"
        else:
            # No parameters, but we already added return type
            pass
        
        return mangled

    def _get_actual_mangled_name(self, function: Dict[str, Any], symbols_by_name: Dict[str, Any]) -> str:
        """Get actual mangled name from symbol table or generate one"""
        func_name = function['Name']
        
        # First try to find exact match in symbol table
        if func_name in symbols_by_name:
            return symbols_by_name[func_name]['mangled_name']
        
        # For template functions, try to find instantiated versions
        if function.get('TemplateParameters'):
            resolved_args = function.get('ResolvedTemplateArgs', [])
            if resolved_args:
                # Look for template instantiation in symbol table
                for symbol_name, symbol_info in symbols_by_name.items():
                    # Check if this symbol matches our template function
                    if func_name in symbol_name and any(arg in symbol_name for arg in resolved_args):
                        return symbol_info['mangled_name']
        
        # If not found in symbol table, generate mangled name
        return self._generate_mangled_name(function)

    def _encode_binary_data(self, file_path: str) -> str:
        """Encode binary file as base64"""
        with open(file_path, 'rb') as f:
            binary_data = f.read()
        return base64.b64encode(binary_data).decode('ascii')

    def _detect_binary_format(self, file_path: str) -> str:
        """Detect binary format from file"""
        path = Path(file_path)
        suffix = path.suffix.lower()
        
        format_map = {
            '.so': 'SO',
            '.dylib': 'DYLIB', 
            '.dll': 'DLL',
            '.a': 'A',
            '.lib': 'LIB'
        }
        
        return format_map.get(suffix, 'SO')

    def compile_source_to_binary(self, source_file: str, 
                                output_dir: Optional[str] = None,
                                include_dirs: List[str] = None,
                                compiler: str = None,
                                optimization: str = "O2",
                                shared: bool = True) -> str:
        """Compile C++ source file to binary library"""
        source_path = Path(source_file)
        include_dirs = include_dirs or []
        compiler = compiler or self._get_default_compiler()
        
        if not self.available_tools.get(compiler, False):
            raise RuntimeError(f"Compiler {compiler} not available")
        
        # Determine output directory and filename
        if output_dir:
            out_dir = Path(output_dir)
            out_dir.mkdir(exist_ok=True)
        else:
            out_dir = source_path.parent
        
        # Generate output filename based on platform
        base_name = source_path.stem
        if shared:
            if self.platform_info['system'] == 'Windows':
                if compiler == 'cl.exe':
                    output_file = out_dir / f"{base_name}.dll"
                else:
                    output_file = out_dir / f"{base_name}.dll"
            elif self.platform_info['system'] == 'Darwin':
                output_file = out_dir / f"lib{base_name}.dylib"
            else:  # Linux and others
                output_file = out_dir / f"lib{base_name}.so"
        else:
            if self.platform_info['system'] == 'Windows':
                if compiler == 'cl.exe':
                    output_file = out_dir / f"{base_name}.lib"
                else:
                    output_file = out_dir / f"{base_name}.a"
            elif self.platform_info['system'] == 'Darwin':
                output_file = out_dir / f"lib{base_name}.a"
            else:  # Linux and others
                output_file = out_dir / f"lib{base_name}.a"
        
        # Build compilation command
        if compiler in ['g++', 'clang++']:
            cmd = [compiler, f'-{optimization}', '-c']  # Compile to object file first
            
            # Add include directories
            for inc_dir in include_dirs:
                cmd.extend(['-I', inc_dir])
            
            # Add standard flags for library compilation
            cmd.extend(['-fPIC', '-std=c++17'])  # Position independent code
            
            # Object file name
            obj_file = out_dir / f"{base_name}.o"
            cmd.extend(['-o', str(obj_file), str(source_file)])
            
            print(f"Compiling {source_file} to object file...")
            print(f"Command: {' '.join(cmd)}")
            
            try:
                result = subprocess.run(cmd, capture_output=True, text=True, check=True)
                print("Compilation successful")
                
                # Now create shared library or archive
                if shared:
                    # Create shared library
                    link_cmd = [compiler, '-shared', '-o', str(output_file), str(obj_file)]
                    print(f"Creating shared library: {' '.join(link_cmd)}")
                    subprocess.run(link_cmd, check=True)
                    
                    # Clean up object file
                    obj_file.unlink()
                else:
                    # Create static archive
                    if self.available_tools.get('ar'):
                        ar_cmd = ['ar', 'rcs', str(output_file), str(obj_file)]
                        print(f"Creating archive: {' '.join(ar_cmd)}")
                        subprocess.run(ar_cmd, check=True)
                        
                        # Clean up object file
                        obj_file.unlink()
                    else:
                        # If no ar available, just rename object file
                        obj_file.rename(output_file)
                        print(f"No ar tool available, using object file as library")
                
            except subprocess.CalledProcessError as e:
                raise RuntimeError(f"Compilation failed: {e.stderr}")
                
        elif compiler == 'cl.exe':
            # Windows MSVC compilation
            cmd = ['cl.exe', '/c', f'/{optimization}', '/std:c++17']
            
            # Add include directories
            for inc_dir in include_dirs:
                cmd.append(f'/I{inc_dir}')
            
            # Object file
            obj_file = out_dir / f"{base_name}.obj"
            cmd.extend([f'/Fo{obj_file}', str(source_file)])
            
            print(f"Compiling with MSVC: {' '.join(cmd)}")
            
            try:
                subprocess.run(cmd, check=True)
                
                # Create library with lib.exe
                if self.available_tools.get('lib'):
                    lib_cmd = ['lib', f'/OUT:{output_file}', str(obj_file)]
                    subprocess.run(lib_cmd, check=True)
                    obj_file.unlink()
                else:
                    obj_file.rename(output_file)
                    
            except subprocess.CalledProcessError as e:
                raise RuntimeError(f"MSVC compilation failed: {e}")
        else:
            raise RuntimeError(f"Unsupported compiler: {compiler}")
        
        if not output_file.exists():
            raise RuntimeError(f"Compilation did not produce expected output: {output_file}")
        
        print(f"Successfully compiled to: {output_file}")
        return str(output_file)

    def compile_multiple_sources_to_library(self, source_files: List[str],
                                          library_name: str,
                                          output_dir: Optional[str] = None,
                                          include_dirs: List[str] = None,
                                          compiler: str = None,
                                          shared: bool = True) -> str:
        """Compile multiple C++ source files into a single library"""
        include_dirs = include_dirs or []
        compiler = compiler or self._get_default_compiler()
        
        if not source_files:
            raise ValueError("No source files provided")
        
        # Determine output directory
        if output_dir:
            out_dir = Path(output_dir)
            out_dir.mkdir(exist_ok=True)
        else:
            out_dir = Path(source_files[0]).parent
        
        # Determine library filename
        if shared:
            if self.platform_info['system'] == 'Windows':
                lib_file = out_dir / f"{library_name}.dll"
            elif self.platform_info['system'] == 'Darwin':
                lib_file = out_dir / f"lib{library_name}.dylib"
            else:
                lib_file = out_dir / f"lib{library_name}.so"
        else:
            if self.platform_info['system'] == 'Windows':
                lib_file = out_dir / f"{library_name}.lib"
            else:
                lib_file = out_dir / f"lib{library_name}.a"
        
        # Compile all sources to object files
        object_files = []
        
        try:
            for source_file in source_files:
                source_path = Path(source_file)
                obj_file = out_dir / f"{source_path.stem}.o"
                
                if compiler in ['g++', 'clang++']:
                    cmd = [compiler, '-O2', '-c', '-fPIC', '-std=c++17']
                    
                    for inc_dir in include_dirs:
                        cmd.extend(['-I', inc_dir])
                    
                    cmd.extend(['-o', str(obj_file), str(source_file)])
                    
                    print(f"Compiling {source_file}...")
                    subprocess.run(cmd, capture_output=True, text=True, check=True)
                    object_files.append(obj_file)
                    
                elif compiler == 'cl.exe':
                    obj_file = out_dir / f"{source_path.stem}.obj"
                    cmd = ['cl.exe', '/c', '/O2', '/std:c++17']
                    
                    for inc_dir in include_dirs:
                        cmd.append(f'/I{inc_dir}')
                    
                    cmd.extend([f'/Fo{obj_file}', str(source_file)])
                    subprocess.run(cmd, check=True)
                    object_files.append(obj_file)
            
            # Link into library
            if shared:
                # Create shared library
                if compiler in ['g++', 'clang++']:
                    link_cmd = [compiler, '-shared', '-o', str(lib_file)]
                    link_cmd.extend(str(obj) for obj in object_files)
                    subprocess.run(link_cmd, check=True)
                elif compiler == 'cl.exe':
                    link_cmd = ['link', '/DLL', f'/OUT:{lib_file}']
                    link_cmd.extend(str(obj) for obj in object_files)
                    subprocess.run(link_cmd, check=True)
            else:
                # Create static library
                if self.available_tools.get('ar') and compiler != 'cl.exe':
                    ar_cmd = ['ar', 'rcs', str(lib_file)]
                    ar_cmd.extend(str(obj) for obj in object_files)
                    subprocess.run(ar_cmd, check=True)
                elif self.available_tools.get('lib') and compiler == 'cl.exe':
                    lib_cmd = ['lib', f'/OUT:{lib_file}']
                    lib_cmd.extend(str(obj) for obj in object_files)
                    subprocess.run(lib_cmd, check=True)
                else:
                    raise RuntimeError("No archiver tool available for static library creation")
            
            # Clean up object files
            for obj_file in object_files:
                if obj_file.exists():
                    obj_file.unlink()
            
            print(f"Successfully created library: {lib_file}")
            return str(lib_file)
            
        except subprocess.CalledProcessError as e:
            # Clean up on failure
            for obj_file in object_files:
                if obj_file.exists():
                    obj_file.unlink()
            raise RuntimeError(f"Library compilation failed: {e}")

    def _is_source_file(self, file_path: str) -> bool:
        """Check if file is a C++ source file that should be compiled"""
        suffix = Path(file_path).suffix.lower()
        return suffix in ['.cpp', '.cxx', '.cc', '.c++', '.c']

    def _is_header_file(self, file_path: str) -> bool:
        """Check if file is a header file"""
        suffix = Path(file_path).suffix.lower()
        return suffix in ['.h', '.hpp', '.hxx', '.h++', '.hh']

    def create_shared_library(self, source_files: List[str],
                            library_name: str,
                            output_dir: Optional[str] = None,
                            include_dirs: List[str] = None,
                            compiler: str = None,
                            optimization: str = "O2",
                            additional_flags: List[str] = None) -> str:
        """Create shared library with advanced options"""
        additional_flags = additional_flags or []
        include_dirs = include_dirs or []
        compiler = compiler or self._get_default_compiler()
        
        if output_dir:
            out_dir = Path(output_dir)
            out_dir.mkdir(exist_ok=True)
        else:
            out_dir = Path(source_files[0]).parent
        
        # Determine shared library name
        if self.platform_info['system'] == 'Windows':
            lib_file = out_dir / f"{library_name}.dll"
        elif self.platform_info['system'] == 'Darwin':
            lib_file = out_dir / f"lib{library_name}.dylib"
        else:
            lib_file = out_dir / f"lib{library_name}.so"
        
        if compiler in ['g++', 'clang++']:
            cmd = [compiler, '-shared', f'-{optimization}', '-fPIC', '-std=c++17']
            
            # Add include directories
            for inc_dir in include_dirs:
                cmd.extend(['-I', inc_dir])
            
            # Add additional flags
            cmd.extend(additional_flags)
            
            # Add source files and output
            cmd.extend(['-o', str(lib_file)])
            cmd.extend(source_files)
            
            print(f"Creating shared library: {' '.join(cmd)}")
            subprocess.run(cmd, check=True)
            
        elif compiler == 'cl.exe':
            # MSVC shared library creation
            obj_files = []
            
            # First compile to object files
            for source_file in source_files:
                obj_file = out_dir / f"{Path(source_file).stem}.obj"
                compile_cmd = ['cl.exe', '/c', f'/{optimization}', '/std:c++17']
                
                for inc_dir in include_dirs:
                    compile_cmd.append(f'/I{inc_dir}')
                
                compile_cmd.extend([f'/Fo{obj_file}', source_file])
                subprocess.run(compile_cmd, check=True)
                obj_files.append(obj_file)
            
            # Link to DLL
            link_cmd = ['link', '/DLL', f'/OUT:{lib_file}']
            link_cmd.extend(additional_flags)
            link_cmd.extend(str(obj) for obj in obj_files)
            subprocess.run(link_cmd, check=True)
            
            # Cleanup
            for obj in obj_files:
                obj.unlink()
        
        return str(lib_file)

    def analyze_dependencies(self, source_files: List[str]) -> List[Dict[str, Any]]:
        """Analyze #include dependencies in source files"""
        dependencies = []
        include_pattern = r'^\s*#include\s*[<"]([^>"]+)[>"]'
        
        for source_file in source_files:
            try:
                with open(source_file, 'r', encoding='utf-8') as f:
                    content = f.read()
                
                includes = re.findall(include_pattern, content, re.MULTILINE)
                for include in includes:
                    # Skip standard library headers
                    if not any(include.startswith(std) for std in ['iostream', 'vector', 'string', 'memory', 'algorithm']):
                        # Try to determine if it's a system or local header
                        dep_info = {
                            "Name": include,
                            "Version": "unknown",
                            "Optional": False,
                            "Description": f"Header dependency from {Path(source_file).name}"
                        }
                        
                        # Avoid duplicates
                        if not any(d["Name"] == include for d in dependencies):
                            dependencies.append(dep_info)
                            
            except Exception as e:
                print(f"Warning: Could not analyze dependencies in {source_file}: {e}")
        
        return dependencies

    def generate_build_info(self, source_files: List[str], 
                          compiler: str, 
                          include_dirs: List[str],
                          optimization: str = "O2") -> Dict[str, Any]:
        """Generate detailed build information"""
        compiler = compiler or self._get_default_compiler()
        
        # Get compiler version
        try:
            version_result = subprocess.run([compiler, '--version'], 
                                          capture_output=True, text=True)
            compiler_version = version_result.stdout.split('\n')[0] if version_result.returncode == 0 else "unknown"
        except:
            compiler_version = "unknown"
        
        # Detect available targets/architectures
        available_targets = []
        available_architectures = []
        
        if compiler in ['g++', 'clang++']:
            try:
                # Get supported targets
                targets_result = subprocess.run([compiler, '--print-targets'], 
                                              capture_output=True, text=True)
                if targets_result.returncode == 0:
                    available_targets = [line.strip() for line in targets_result.stdout.split('\n') if line.strip()]
                
                # Common architectures
                available_architectures = ['x86_64', 'i386', 'arm64', 'armv7']
            except:
                pass
        
        build_flags = [f'-{optimization}', '-std=c++17']
        if compiler in ['g++', 'clang++']:
            build_flags.extend(['-fPIC', '-Wall'])
        
        return {
            "Compiler": compiler,
            "CompilerVersion": compiler_version,
            "BuildFlags": build_flags,
            "OptimizationLevel": optimization,
            "DebugSymbols": False,
            "AvailableTargets": available_targets[:10],  # Limit output size
            "AvailableArchitectures": available_architectures
        }

    def extract_template_info(self, preprocessed_source: str) -> List[Dict[str, Any]]:
        """Extract template function information"""
        template_functions = []
        
        # Pattern for template declarations
        template_pattern = r'template\s*<([^>]+)>\s*(?:(?:inline|static|extern)\s+)*(\w+(?:\s*<[^>]*>)?)\s+(\w+)\s*\([^)]*\)'
        
        matches = re.finditer(template_pattern, preprocessed_source, re.MULTILINE | re.DOTALL)
        
        for match in matches:
            template_params_str = match.group(1)
            return_type = match.group(2)
            func_name = match.group(3)
            
            # Parse template parameters
            template_params = []
            param_parts = template_params_str.split(',')
            
            for param in param_parts:
                param = param.strip()
                if param.startswith('typename') or param.startswith('class'):
                    param_type = 'typename' if param.startswith('typename') else 'class'
                    param_name = param.split()[-1] if len(param.split()) > 1 else 'T'
                    
                    template_params.append({
                        "Name": param_name,
                        "Type": param_type,
                        "Description": f"Template type parameter"
                    })
                elif any(param.startswith(t) for t in ['int', 'size_t', 'bool']):
                    parts = param.split()
                    param_type = parts[0]
                    param_name = parts[1] if len(parts) > 1 else 'N'
                    
                    template_params.append({
                        "Name": param_name,
                        "Type": param_type,
                        "Description": f"Template value parameter"
                    })
            
            if template_params:  # Only add if we found template parameters
                template_functions.append({
                    "Name": func_name,
                    "ReturnType": return_type,
                    "TemplateParameters": template_params
                })
        
        return template_functions

    def extract_template_instantiations(self, preprocessed_source: str) -> List[Dict[str, Any]]:
        """Extract template instantiations and their resolved types"""
        instantiations = []
        
        # Look for lines that contain template instantiations
        lines = preprocessed_source.split('\n')
        for line in lines:
            line = line.strip()
            # Look for pattern: function_name<type>(args);
            if ';' in line and '<' in line and '>' in line and '(' in line and ')' in line:
                # More specific pattern for template function calls
                match = re.search(r'(\w+)\s*<([^>]+)>\s*\(([^)]*)\)\s*;', line)
                if match:
                    func_name = match.group(1)
                    template_args_str = match.group(2)
                    call_params_str = match.group(3)
                    
                    # Skip if this looks like a template declaration
                    if 'template' in line or 'typename' in line or 'class' in line:
                        continue
                    
                    # Parse template arguments
                    template_args = []
                    arg_parts = template_args_str.split(',')
                    for arg in arg_parts:
                        arg = arg.strip()
                        if arg and arg not in ['typename', 'class']:
                            template_args.append(arg)
                    
                    # Only add if we have valid template arguments
                    if template_args:
                        # Parse call parameters to see resolved types and infer deduced template arguments
                        call_params = []
                        deduced_args = []
                        if call_params_str.strip():
                            param_parts = call_params_str.split(',')
                            for i, param in enumerate(param_parts):
                                param = param.strip()
                                if param and param != 'nullptr':
                                    # Try to infer type from the parameter
                                    param_type = self._infer_parameter_type(param)
                                    call_params.append({
                                        "Index": i,
                                        "Value": param,
                                        "InferredType": param_type
                                    })
                                    # Add deduced template argument
                                    deduced_args.append(param_type)
                        
                        instantiations.append({
                            "FunctionName": func_name,
                            "TemplateArguments": template_args,
                            "DeducedArguments": deduced_args,
                            "CallParameters": call_params,
                            "ResolvedSignature": f"{func_name}<{template_args_str}>({call_params_str})"
                        })
        
        return instantiations

    def _infer_parameter_type(self, param_value: str) -> str:
        """Infer parameter type from its value"""
        param_value = param_value.strip()
        
        # Remove common prefixes/suffixes
        if param_value.startswith('"') and param_value.endswith('"'):
            return 'const char*'
        elif param_value.isdigit():
            return 'int'
        elif param_value.replace('.', '').replace('f', '').replace('F', '').isdigit():
            if 'f' in param_value.lower():
                return 'float'
            else:
                return 'double'
        elif param_value.lower() in ['true', 'false']:
            return 'bool'
        elif param_value == 'nullptr':
            return 'void*'
        elif param_value.startswith('{') and param_value.endswith('}'):
            return 'array'
        else:
            return 'unknown'

    def resolve_template_types(self, function: Dict[str, Any], template_args: List[str]) -> Dict[str, Any]:
        """Resolve template types in function parameters"""
        resolved_function = function.copy()
        resolved_parameters = []
        
        # Get template parameter names from the function's template parameters
        template_param_names = []
        for template_param in function.get('TemplateParameters', []):
            template_param_names.append(template_param['Name'])
        
        for param in function.get('Parameters', []):
            resolved_param = param.copy()
            param_type = param['Type']
            original_type = param_type
            
            # Replace template parameters with actual types
            for i, template_arg in enumerate(template_args):
                if i < len(template_param_names):
                    template_param_name = template_param_names[i]
                    # Replace the template parameter name with the actual type
                    if template_param_name in param_type:
                        resolved_param['Type'] = param_type.replace(template_param_name, template_arg)
                        resolved_param['BaseType'] = template_arg
                        resolved_param['ResolvedFromTemplate'] = True
                        resolved_param['OriginalTemplateType'] = original_type
                        param_type = resolved_param['Type']  # Update for next iteration
                        break
            
            resolved_parameters.append(resolved_param)
        
        resolved_function['Parameters'] = resolved_parameters
        resolved_function['ResolvedTemplateArgs'] = template_args
        return resolved_function

    def generate_usage_examples(self, functions: List[Dict[str, Any]], 
                              library_name: str) -> Dict[str, List[Dict[str, Any]]]:
        """Generate usage examples for functions"""
        examples = {}
        
        for func in functions[:5]:  # Limit to first 5 functions
            func_name = func['Name']
            params = func.get('Parameters', [])
            return_type = func.get('ReturnType', 'void')
            
            # Generate C++ example
            param_list = ', '.join([f"{p['Type']} {p['Name']}" for p in params])
            call_params = ', '.join([self._generate_example_value(p['Type']) for p in params])
            
            cpp_example = f"""#include "{library_name.lower()}.h"

int main() {{
    // Call {func_name}
    {return_type} result = {func_name}({call_params});
    return 0;
}}"""

            examples[func_name] = [{
                "Language": "C++",
                "Code": cpp_example,
                "Description": f"Basic usage of {func_name} function"
            }]
        
        return examples

    def _generate_example_value(self, param_type: str) -> str:
        """Generate example values for different parameter types"""
        param_type_lower = param_type.lower()
        
        if 'int' in param_type_lower:
            return '42'
        elif 'float' in param_type_lower or 'double' in param_type_lower:
            return '3.14f' if 'float' in param_type_lower else '3.14'
        elif 'string' in param_type_lower:
            return '"example"'
        elif 'char' in param_type_lower and '*' in param_type:
            return '"hello"'
        elif 'bool' in param_type_lower:
            return 'true'
        elif '*' in param_type:
            return 'nullptr'
        else:
            return f'{param_type}()'
        """Detect binary format from file"""
        path = Path(file_path)
        suffix = path.suffix.lower()
        
        format_map = {
            '.so': 'SO',
            '.dylib': 'DYLIB', 
            '.dll': 'DLL',
            '.a': 'A',
            '.lib': 'LIB'
        }
        
        return format_map.get(suffix, 'SO')

    def build_catalog(self, source_files: List[str],
                     library_name: str = "C++ Library",
                     library_version: str = "1.0.0",
                     include_dirs: List[str] = None,
                     output_file: Optional[str] = None,
                     temp_dir: Optional[str] = None,
                     compiler: str = None) -> Dict[str, Any]:
        """Build complete catalog from source files (auto-detecting headers vs binaries)"""
        import builtins
        
        # Categorize input files
        header_files = []
        library_files = []
        source_files_to_compile = []
        compiled_libraries = []  # Track libraries we create for cleanup
        
        # Create temporary directory for compiled binaries if needed
        if temp_dir:
            temp_build_dir = Path(temp_dir)
            temp_build_dir.mkdir(exist_ok=True)
        else:
            temp_build_dir = None
        
        print("Categorizing input files...")
        for file_path in source_files:
            path = Path(file_path)
            if not path.exists():
                print(f"Warning: File {file_path} not found, skipping...")
                continue
                
            suffix = path.suffix.lower()
            
            # Header files
            if self._is_header_file(file_path):
                header_files.append(file_path)
                print(f"  Header: {file_path}")
            # Binary library files
            elif suffix in ['.so', '.a', '.dylib', '.dll', '.lib']:
                library_files.append(file_path)
                print(f"  Binary: {file_path}")
            # Source files that need compilation
            elif self._is_source_file(file_path):
                source_files_to_compile.append(file_path)
                print(f"  Source (will compile): {file_path}")
            else:
                print(f"Warning: Unknown file type {file_path}, treating as header...")
                header_files.append(file_path)
        
        # Compile source files to binaries
        if source_files_to_compile:
            print(f"\nCompiling {len(source_files_to_compile)} source files...")
            
            if len(source_files_to_compile) == 1:
                # Single source file
                try:
                    compiled_lib = self.compile_source_to_binary(
                        source_files_to_compile[0],
                        output_dir=str(temp_build_dir) if temp_build_dir else None,
                        include_dirs=include_dirs,
                        compiler=compiler,
                        shared=not getattr(builtins.args, 'static', False) if hasattr(builtins, 'args') else True
                    )
                    library_files.append(compiled_lib)
                    compiled_libraries.append(compiled_lib)
                except Exception as e:
                    print(f"Warning: Failed to compile {source_files_to_compile[0]}: {e}")
            else:
                # Multiple source files - create combined library
                try:
                    compiled_lib = self.compile_multiple_sources_to_library(
                        source_files_to_compile,
                        library_name.lower().replace(' ', '_'),
                        output_dir=str(temp_build_dir) if temp_build_dir else None,
                        include_dirs=include_dirs,
                        compiler=compiler,
                        shared=not getattr(builtins.args, 'static', False) if hasattr(builtins, 'args') else True
                    )
                    library_files.append(compiled_lib)
                    compiled_libraries.append(compiled_lib)
                except Exception as e:
                    print(f"Warning: Failed to compile sources into library: {e}")
        
        if not header_files and not library_files:
            raise ValueError("No valid header or library files found after processing")
        
        print(f"\nFinal file counts:")
        print(f"  Headers/Sources for analysis: {len(header_files + source_files_to_compile)}")
        print(f"  Binary libraries: {len(library_files)}")
        
        try:
            # Continue with existing catalog building logic
            catalog = {
                "Catalog": {
                    "Version": "1.0.0", 
                    "Created": datetime.now().isoformat(),
                    "Updated": datetime.now().isoformat(),
                    "Description": f"Binary catalog for {library_name}"
                },
                "Libraries": []
            }
            
            all_functions = []
            all_architectures = []
            symbols_by_name = {}
            
            # Process headers and source files for function signatures
            analysis_files = header_files + source_files_to_compile
            if analysis_files:
                print("\nProcessing files for function extraction...")
                # Enhanced function processing with template info
                for analysis_file in analysis_files:
                    print(f"  Analyzing {analysis_file}...")
                    try:
                        preprocessed = self.analyze_header_with_preprocessor(analysis_file, include_dirs)
                        functions = self.extract_functions_from_preprocessed(preprocessed)
                        
                        # Extract template information
                        template_functions = self.extract_template_info(preprocessed['expanded_source'])
                        
                        # Extract template instantiations
                        template_instantiations = self.extract_template_instantiations(preprocessed['expanded_source'])
                        
                        # Merge template info with functions
                        for func in functions:
                            for template_func in template_functions:
                                if func['Name'] == template_func['Name']:
                                    func['TemplateParameters'] = template_func.get('TemplateParameters', [])
                                    break
                        
                        # Process template instantiations and show resolved types
                        if template_instantiations:
                            print(f"    Found {len(template_instantiations)} template instantiations:")
                            for instantiation in template_instantiations:
                                func_name = instantiation['FunctionName']
                                template_args = instantiation['TemplateArguments']
                                deduced_args = instantiation.get('DeducedArguments', [])
                                print(f"      {func_name}<{', '.join(template_args)}>")
                                
                                # Find the corresponding template function and resolve types
                                for i, func in enumerate(functions):
                                    if func['Name'] == func_name and func.get('TemplateParameters'):
                                        # Combine explicit and deduced template arguments
                                        all_template_args = template_args + deduced_args
                                        resolved_func = self.resolve_template_types(func, all_template_args)
                                        
                                        # Update the original function with resolved types
                                        functions[i] = resolved_func
                                        
                                        print(f"        Resolved signature: {resolved_func['ReturnType']} {func_name}<{', '.join(template_args)}>(", end="")
                                        param_types = []
                                        for param in resolved_func['Parameters']:
                                            if param.get('ResolvedFromTemplate'):
                                                param_types.append(f"{param['Type']} {param['Name']} (resolved from {param['OriginalTemplateType']})")
                                            else:
                                                param_types.append(f"{param['Type']} {param['Name']}")
                                        print(', '.join(param_types) + ")")
                                        break
                        
                        all_functions.extend(functions)
                        print(f"    Found {len(functions)} functions ({len(template_functions)} templates)")
                    except Exception as e:
                        print(f"    Error processing {analysis_file}: {e}")
            
            # Process binary libraries if any
            if library_files:
                print("\nProcessing binary libraries...")
                for lib_file in library_files:
                    print(f"  Analyzing {lib_file}...")
                    try:
                        binary_info = self.analyze_binary_library(lib_file)
                        
                        # Create architecture entry
                        arch_info = {
                            "Name": self.platform_info['host_arch'].lower(),
                            "HostArch": self.platform_info['host_arch'],
                            "Platforms": [self.platform_info['platform']],
                            "BinaryFormat": binary_info['binary_format'],
                            "BinaryData": binary_info['binary_data'],
                            "FileSize": binary_info['file_size'],
                            "Checksum": binary_info['checksums']['SHA256']
                        }
                        all_architectures.append(arch_info)
                        
                        # Index symbols by demangled name for matching
                        for symbol in binary_info['symbols']:
                            demangled = symbol.get('demangled_name', symbol['name'])
                            # Extract just the function name without parameters/namespace
                            func_name = demangled.split('(')[0].split('::')[-1]
                            symbols_by_name[func_name] = symbol
                        
                        print(f"    Found {len(binary_info['symbols'])} symbols")
                        
                    except Exception as e:
                        print(f"    Error processing {lib_file}: {e}")
            else:
                # Create a default architecture entry for source-only analysis
                print("No binary libraries found, creating default architecture entry...")
                arch_info = {
                    "Name": self.platform_info['host_arch'].lower(),
                    "HostArch": self.platform_info['host_arch'],
                    "Platforms": [self.platform_info['platform']],
                    "BinaryFormat": self.platform_info['binary_format'],
                    "FileSize": 0,
                    "Checksum": {
                        "Algorithm": "SHA256",
                        "Value": "0000000000000000000000000000000000000000000000000000000000000000"
                    }
                }
                all_architectures.append(arch_info)
            
            # Enhanced build info generation
            build_info = self.generate_build_info(
                source_files_to_compile, 
                compiler or self._get_default_compiler(),
                include_dirs or []
            )
            
            # Analyze dependencies
            dependencies = []
            if source_files_to_compile:
                dependencies = self.analyze_dependencies(source_files_to_compile)
            
            # Generate examples if requested
            examples_data = {}
            if hasattr(args, 'examples') and getattr(args, 'examples', False) and 'args' in locals():
                examples_data = self.generate_usage_examples(all_functions, library_name)
            # Set mangled names for all functions
            print("\nSetting mangled names for functions...")
            for function in all_functions:
                mangled_name = self._get_actual_mangled_name(function, symbols_by_name)
                function['Symbol'] = mangled_name
                
                # Show what we found/generated
                if function['Name'] in symbols_by_name:
                    print(f"  Found symbol for {function['Name']} -> {mangled_name}")
                else:
                    print(f"  Generated mangled name for {function['Name']} -> {mangled_name}")
            
            # Build library entry
            library_entry = {
                "Id": f"{library_name.lower().replace(' ', '-')}-{library_version}",
                "Name": library_name,
                "Version": library_version,
                "Description": f"C++ library with {len(all_functions)} functions",
                "Vendor": "Generated",
                "License": "Unknown",
                "Categories": self._determine_categories(all_functions),
                "Architectures": all_architectures,
                "Functions": all_functions,
                "Dependencies": dependencies,
                "BuildInfo": build_info,
                "Metadata": {
                    "SourceFile": source_files[0] if source_files else "",
                    "PreprocessorUsed": True,
                    "TotalKernels": 0,
                    "KernelNames": [],
                    "CompiledSources": len(source_files_to_compile),
                    "CompiledLibraries": compiled_libraries
                }
            }
            
            catalog["Libraries"].append(library_entry)
            
            # Write output
            if output_file:
                with open(output_file, 'w', encoding='utf-8') as f:
                    json.dump(catalog, f, indent=2)
                print(f"\nCatalog written to {output_file}")
            else:
                # Print to stdout if no output file specified
                print("\n" + "="*50)
                print("GENERATED CATALOG:")
                print("="*50)
                print(json.dumps(catalog, indent=2))
            
            return catalog
            
        finally:
            # Cleanup compiled libraries if they were temporary
            if not output_file and compiled_libraries:
                print(f"\nCleaning up {len(compiled_libraries)} temporary compiled libraries...")
                for lib_path in compiled_libraries:
                    try:
                        Path(lib_path).unlink()
                        print(f"  Deleted: {lib_path}")
                    except Exception as e:
                        print(f"  Warning: Could not delete {lib_path}: {e}")

    def _determine_categories(self, functions: List[Dict[str, Any]]) -> List[str]:
        """Determine library categories based on function names"""
        categories = set()
        
        for func in functions:
            name = func['Name'].lower()
            
            # Categorize based on common patterns
            if any(term in name for term in ['math', 'calc', 'compute', 'algorithm']):
                categories.add('Compute')
            elif any(term in name for term in ['string', 'str', 'text']):
                categories.add('Utility')
            elif any(term in name for term in ['vector', 'matrix', 'linear']):
                categories.add('Compute')
            elif any(term in name for term in ['file', 'io', 'read', 'write']):
                categories.add('System')
            elif any(term in name for term in ['network', 'socket', 'tcp', 'udp']):
                categories.add('Networking')
            elif any(term in name for term in ['thread', 'mutex', 'lock']):
                categories.add('System')
            elif any(term in name for term in ['hash', 'encrypt', 'decrypt', 'crypto']):
                categories.add('Cryptography')
        
        if not categories:
            categories.add('Utility')
        
        return list(categories)


def main():
    from pathlib import Path
    parser = argparse.ArgumentParser(description='Build JSON catalog from C++ source files and libraries')
    parser.add_argument('sources', nargs='+', help='C++ source files (.h, .hpp, .cpp, .so, .a, .dylib, .dll, etc.)')
    parser.add_argument('--name', help='Library name (defaults to input file name)')
    parser.add_argument('--version', default='1.0.0', help='Library version')
    parser.add_argument('--include', '-I', action='append', dest='includes', help='Include directories')
    parser.add_argument('--output', '-o', help='Output JSON file (optional - prints to stdout if not specified)')
    parser.add_argument('--static', action='store_true', help='Create static libraries instead of shared (shared is now default)')
    parser.add_argument('--optimization', '-O', default='O2', choices=['O0', 'O1', 'O2', 'O3', 'Os'], help='Optimization level')
    parser.add_argument('--build-flags', action='append', help='Additional compiler flags')
    parser.add_argument('--examples', action='store_true', help='Generate usage examples')
    parser.add_argument('--temp-dir', help='Temporary directory for compiled binaries')
    parser.add_argument('--compiler', help='C++ compiler to use (g++, clang++, cl.exe)')
    
    args = parser.parse_args()
    
    # Generate library name from input files if not provided
    if not args.name:
        if len(args.sources) == 1:
            # Single file - use the filename without extension
            input_file = Path(args.sources[0])
            args.name = input_file.stem
        else:
            # Multiple files - create a combined name
            file_names = [Path(f).stem for f in args.sources]
            # Remove duplicates and sort
            unique_names = sorted(list(set(file_names)))
            if len(unique_names) == 1:
                args.name = unique_names[0]
            else:
                # Create a combined name from first few files
                if len(unique_names) <= 3:
                    args.name = "_".join(unique_names)
                else:
                    args.name = f"{unique_names[0]}_and_{len(unique_names)-1}_more"
    
    builder = CppLibraryCatalogBuilder()
    
    try:
        # Make args available to build_catalog method
        import builtins
        builtins.args = args
        
        catalog = builder.build_catalog(
            source_files=args.sources,
            library_name=args.name,
            library_version=args.version,
            include_dirs=args.includes or [],
            output_file=args.output,
            temp_dir=args.temp_dir,
            compiler=args.compiler
        )
        
        print(f"\n{'='*50}")
        print("CATALOG SUMMARY:")
        print("="*50)
        print(f"Libraries: {len(catalog['Libraries'])}")
        for lib in catalog['Libraries']:
            print(f"  {lib['Name']} v{lib['Version']}")
            print(f"    Functions: {len(lib['Functions'])}")
            template_count = sum(1 for f in lib['Functions'] if f.get('TemplateParameters'))
            if template_count:
                print(f"    Templates: {template_count}")
            print(f"    Architectures: {len(lib['Architectures'])}")
            print(f"    Categories: {', '.join(lib['Categories'])}")
            print(f"    Dependencies: {len(lib.get('Dependencies', []))}")
            if lib['Metadata'].get('CompiledSources', 0) > 0:
                print(f"    Compiled Sources: {lib['Metadata']['CompiledSources']}")
        
        return 0
        
    except Exception as e:
        print(f"Error building catalog: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == '__main__':
    exit(main())
