#!/usr/bin/env python3
"""
CPU Library Catalog Builder

This tool analyzes compiled CPU libraries and generates a JSON catalog
with binary data encoded in Base64 format using the PascalCase schema.

Requirements:
- objdump, nm, readelf (Linux/Unix)
- dumpbin (Windows)
- otool (macOS)
- Python 3.6+

Usage:
    python cpu_catalog_builder.py <library_file> [options]
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
from typing import Dict, List, Optional, Tuple, Set
from datetime import datetime
import platform
import shutil

class CPULibraryAnalyzer:
    """Analyzer for extracting information from compiled CPU libraries."""
    
    def __init__(self, library_path: str):
        self.library_path = Path(library_path)
        self.platform = platform.system().lower()
        self.arch = platform.machine().lower()
        
        # Detect library type and format
        self.library_type = self._detect_library_type()
        self.binary_format = self._detect_binary_format()
        
        # Find available analysis tools
        self.analysis_tools = self._find_analysis_tools()
        
        if not self.analysis_tools:
            raise RuntimeError("No suitable binary analysis tools found")
    
    def _detect_library_type(self) -> str:
        """Detect whether library is static or shared."""
        suffix = self.library_path.suffix.lower()
        
        if suffix in ['.so', '.dylib', '.dll']:
            return "shared"
        elif suffix in ['.a', '.lib']:
            return "static"
        else:
            # Try to detect by file content
            try:
                with open(self.library_path, 'rb') as f:
                    header = f.read(16)
                
                # ELF magic
                if header.startswith(b'\x7fELF'):
                    return "shared" if b'\x03\x00' in header[:8] else "static"
                # PE magic
                elif header.startswith(b'MZ'):
                    return "shared"
                # Mach-O magic
                elif header.startswith((b'\xfe\xed\xfa\xce', b'\xce\xfa\xed\xfe', 
                                      b'\xfe\xed\xfa\xcf', b'\xcf\xfa\xed\xfe')):
                    return "shared"
                # AR archive magic
                elif header.startswith(b'!<arch>\n'):
                    return "static"
                
            except:
                pass
        
        return "unknown"
    
    def _detect_binary_format(self) -> str:
        """Detect binary format based on file and platform."""
        suffix = self.library_path.suffix.lower()
        
        if self.platform == "windows":
            if suffix == '.dll':
                return "DLL"
            elif suffix == '.lib':
                return "LIB"
        elif self.platform == "darwin":
            if suffix == '.dylib':
                return "DYLIB"
            elif suffix == '.a':
                return "A"
        else:  # Linux and other Unix
            if suffix == '.so' or '.so.' in str(self.library_path):
                return "SO"
            elif suffix == '.a':
                return "A"
        
        return "A"  # Default fallback
    
    def _find_analysis_tools(self) -> Dict[str, str]:
        """Find available binary analysis tools."""
        tools = {}
        
        # Cross-platform tools
        tool_candidates = {
            'objdump': ['objdump', 'x86_64-linux-gnu-objdump', 'llvm-objdump'],
            'nm': ['nm', 'llvm-nm'],
            'readelf': ['readelf', 'llvm-readelf'],
            'strings': ['strings'],
            'file': ['file'],
            'otool': ['otool'],  # macOS
            'dumpbin': ['dumpbin']  # Windows
        }
        
        for tool_name, candidates in tool_candidates.items():
            for candidate in candidates:
                if shutil.which(candidate):
                    tools[tool_name] = candidate
                    break
        
        return tools
    
    def analyze_library(self) -> Dict:
        """Perform comprehensive analysis of the library."""
        analysis = {
            "basic_info": self._get_basic_info(),
            "symbols": self._extract_symbols(),
            "dependencies": self._extract_dependencies(),
            "sections": self._extract_sections(),
            "strings": self._extract_notable_strings(),
            "debug_info": self._extract_debug_info()
        }
        
        return analysis
    
    def _get_basic_info(self) -> Dict:
        """Get basic library information."""
        stat = self.library_path.stat()
        
        info = {
            "file_size": stat.st_size,
            "library_type": self.library_type,
            "binary_format": self.binary_format,
            "platform": self.platform,
            "architecture": self._detect_architecture(),
            "modified_time": datetime.fromtimestamp(stat.st_mtime).isoformat()
        }
        
        # Use 'file' command for additional info
        if 'file' in self.analysis_tools:
            try:
                result = subprocess.run([self.analysis_tools['file'], str(self.library_path)], 
                                      capture_output=True, text=True)
                if result.returncode == 0:
                    info["file_description"] = result.stdout.strip()
            except:
                pass
        
        return info
    
    def _detect_architecture(self) -> str:
        """Detect target architecture of the library."""
        # Try objdump first
        if 'objdump' in self.analysis_tools:
            try:
                result = subprocess.run([self.analysis_tools['objdump'], '-f', str(self.library_path)], 
                                      capture_output=True, text=True)
                if result.returncode == 0:
                    output = result.stdout.lower()
                    if 'x86-64' in output or 'x86_64' in output:
                        return "X86_64"
                    elif 'i386' in output or 'x86' in output:
                        return "X86"
                    elif 'aarch64' in output or 'arm64' in output:
                        return "ARM64"
                    elif 'arm' in output:
                        return "ARM"
                    elif 'mips' in output:
                        return "MIPS64" if '64' in output else "MIPS"
                    elif 'riscv' in output:
                        return "RISCV64" if '64' in output else "RISCV"
            except:
                pass
        
        # Try readelf on Linux
        if 'readelf' in self.analysis_tools:
            try:
                result = subprocess.run([self.analysis_tools['readelf'], '-h', str(self.library_path)], 
                                      capture_output=True, text=True)
                if result.returncode == 0:
                    output = result.stdout.lower()
                    if 'x86-64' in output:
                        return "X86_64"
                    elif 'aarch64' in output:
                        return "ARM64"
                    elif 'arm' in output:
                        return "ARM"
            except:
                pass
        
        # Try otool on macOS
        if 'otool' in self.analysis_tools:
            try:
                result = subprocess.run([self.analysis_tools['otool'], '-h', str(self.library_path)], 
                                      capture_output=True, text=True)
                if result.returncode == 0:
                    output = result.stdout.lower()
                    if 'x86_64' in output:
                        return "X86_64"
                    elif 'arm64' in output:
                        return "ARM64"
            except:
                pass
        
        # Fallback to system architecture
        machine = platform.machine().lower()
        if machine in ['x86_64', 'amd64']:
            return "X86_64"
        elif machine in ['i386', 'i686', 'x86']:
            return "X86"
        elif machine in ['aarch64', 'arm64']:
            return "ARM64"
        elif machine.startswith('arm'):
            return "ARM"
        elif machine.startswith('mips'):
            return "MIPS64" if '64' in machine else "MIPS"
        elif machine.startswith('riscv'):
            return "RISCV64" if '64' in machine else "RISCV"
        else:
            return "Universal"
    
    def _extract_symbols(self) -> List[Dict]:
        """Extract symbol information from the library."""
        symbols = []
        
        # Try nm first (most universal)
        if 'nm' in self.analysis_tools:
            symbols.extend(self._extract_symbols_nm())
        
        # Try objdump for additional info
        if 'objdump' in self.analysis_tools:
            objdump_symbols = self._extract_symbols_objdump()
            # Merge with nm symbols
            symbols = self._merge_symbols(symbols, objdump_symbols)
        
        # Try platform-specific tools
        if self.platform == "darwin" and 'otool' in self.analysis_tools:
            otool_symbols = self._extract_symbols_otool()
            symbols = self._merge_symbols(symbols, otool_symbols)
        elif self.platform == "windows" and 'dumpbin' in self.analysis_tools:
            dumpbin_symbols = self._extract_symbols_dumpbin()
            symbols = self._merge_symbols(symbols, dumpbin_symbols)
        
        return symbols
    
    def _extract_symbols_nm(self) -> List[Dict]:
        """Extract symbols using nm."""
        symbols = []
        
        try:
            # Use different nm flags based on library type
            if self.library_type == "static":
                cmd = [self.analysis_tools['nm'], '-P', '-A', str(self.library_path)]
            else:
                cmd = [self.analysis_tools['nm'], '-D', '-P', str(self.library_path)]
            
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            for line in result.stdout.split('\n'):
                line = line.strip()
                if not line:
                    continue
                
                # Parse nm output format
                parts = line.split()
                if len(parts) >= 3:
                    if self.library_type == "static" and len(parts) >= 4:
                        # Archive format: archive.a:object.o: symbol_name type address
                        archive_obj = parts[0]
                        symbol_name = parts[1]
                        symbol_type = parts[2]
                        address = parts[3] if len(parts) > 3 and parts[3] != '' else "0"
                    else:
                        # Regular format: symbol_name type address
                        symbol_name = parts[0]
                        symbol_type = parts[1]
                        address = parts[2] if len(parts) > 2 and parts[2] != '' else "0"
                        archive_obj = ""
                    
                    symbol_info = {
                        "Name": symbol_name,
                        "Type": self._normalize_symbol_type(symbol_type),
                        "Address": address,
                        "Visibility": self._get_symbol_visibility(symbol_type),
                        "IsFunction": symbol_type.upper() in ['T', 'W'],
                        "IsGlobal": symbol_type.isupper(),
                        "Source": "nm"
                    }
                    
                    if archive_obj:
                        symbol_info["ArchiveObject"] = archive_obj
                    
                    symbols.append(symbol_info)
        
        except Exception as e:
            print(f"Error extracting symbols with nm: {e}")
        
        return symbols
    
    def _extract_symbols_objdump(self) -> List[Dict]:
        """Extract symbols using objdump."""
        symbols = []
        
        try:
            result = subprocess.run([self.analysis_tools['objdump'], '-t', str(self.library_path)], 
                                  capture_output=True, text=True)
            
            for line in result.stdout.split('\n'):
                line = line.strip()
                if not line or line.startswith('SYMBOL TABLE'):
                    continue
                
                # Parse objdump symbol table format
                # address flags section size name
                match = re.match(r'([0-9a-fA-F]+)\s+([lwgud!]*)\s+([^\s]+)\s+([0-9a-fA-F]+)\s+(.+)', line)
                if match:
                    address, flags, section, size, name = match.groups()
                    
                    symbol_info = {
                        "Name": name,
                        "Address": address,
                        "Section": section,
                        "Size": size,
                        "Flags": flags,
                        "IsFunction": 'F' in flags or section == '.text',
                        "IsGlobal": 'g' in flags,
                        "Source": "objdump"
                    }
                    
                    symbols.append(symbol_info)
        
        except Exception as e:
            print(f"Error extracting symbols with objdump: {e}")
        
        return symbols
    
    def _extract_symbols_otool(self) -> List[Dict]:
        """Extract symbols using otool (macOS)."""
        symbols = []
        
        try:
            result = subprocess.run([self.analysis_tools['otool'], '-tV', str(self.library_path)], 
                                  capture_output=True, text=True)
            
            for line in result.stdout.split('\n'):
                # Parse otool output for function symbols
                match = re.match(r'^([0-9a-fA-F]+)\s+(.+)', line)
                if match:
                    address, instruction = match.groups()
                    # Look for function calls and references
                    if 'call' in instruction or 'bl' in instruction:
                        func_match = re.search(r'_(\w+)', instruction)
                        if func_match:
                            func_name = func_match.group(1)
                            
                            symbol_info = {
                                "Name": func_name,
                                "Address": address,
                                "IsFunction": True,
                                "Source": "otool"
                            }
                            
                            symbols.append(symbol_info)
        
        except Exception as e:
            print(f"Error extracting symbols with otool: {e}")
        
        return symbols
    
    def _extract_symbols_dumpbin(self) -> List[Dict]:
        """Extract symbols using dumpbin (Windows)."""
        symbols = []
        
        try:
            result = subprocess.run([self.analysis_tools['dumpbin'], '/SYMBOLS', str(self.library_path)], 
                                  capture_output=True, text=True)
            
            for line in result.stdout.split('\n'):
                # Parse dumpbin symbol output
                # Look for symbol table entries
                match = re.match(r'\s*\d+\s+([0-9A-F]+)\s+\w+\s+\w+\s+\w+\s+\w+\s+(.+)', line)
                if match:
                    address, name = match.groups()
                    
                    symbol_info = {
                        "Name": name.strip(),
                        "Address": address,
                        "IsFunction": True,  # Assume function for now
                        "Source": "dumpbin"
                    }
                    
                    symbols.append(symbol_info)
        
        except Exception as e:
            print(f"Error extracting symbols with dumpbin: {e}")
        
        return symbols
    
    def _merge_symbols(self, symbols1: List[Dict], symbols2: List[Dict]) -> List[Dict]:
        """Merge symbol lists, removing duplicates and combining information."""
        symbol_map = {}
        
        # Add first set of symbols
        for symbol in symbols1:
            name = symbol["Name"]
            symbol_map[name] = symbol.copy()
        
        # Merge second set
        for symbol in symbols2:
            name = symbol["Name"]
            if name in symbol_map:
                # Merge additional information
                existing = symbol_map[name]
                for key, value in symbol.items():
                    if key not in existing or not existing[key]:
                        existing[key] = value
                    elif key == "Source":
                        existing[key] = f"{existing[key]}, {value}"
            else:
                symbol_map[name] = symbol.copy()
        
        return list(symbol_map.values())
    
    def _normalize_symbol_type(self, symbol_type: str) -> str:
        """Normalize symbol type to standard format."""
        type_map = {
            'T': 'Function',
            't': 'LocalFunction',
            'D': 'InitializedData',
            'd': 'LocalInitializedData', 
            'B': 'UninitializedData',
            'b': 'LocalUninitializedData',
            'U': 'Undefined',
            'W': 'WeakFunction',
            'w': 'WeakObject',
            'R': 'ReadOnlyData',
            'r': 'LocalReadOnlyData',
            'C': 'CommonSymbol',
            'A': 'Absolute',
            'S': 'SmallObject',
            'N': 'DebugSymbol'
        }
        
        return type_map.get(symbol_type.upper(), 'Unknown')
    
    def _get_symbol_visibility(self, symbol_type: str) -> str:
        """Determine symbol visibility."""
        if symbol_type.isupper():
            return "Global"
        else:
            return "Local"
    
    def _extract_dependencies(self) -> List[Dict]:
        """Extract library dependencies."""
        dependencies = []
        
        # Use different tools based on platform
        if self.platform == "linux" and 'readelf' in self.analysis_tools:
            dependencies = self._extract_dependencies_readelf()
        elif self.platform == "darwin" and 'otool' in self.analysis_tools:
            dependencies = self._extract_dependencies_otool()
        elif self.platform == "windows" and 'dumpbin' in self.analysis_tools:
            dependencies = self._extract_dependencies_dumpbin()
        elif 'objdump' in self.analysis_tools:
            dependencies = self._extract_dependencies_objdump()
        
        return dependencies
    
    def _extract_dependencies_readelf(self) -> List[Dict]:
        """Extract dependencies using readelf."""
        dependencies = []
        
        try:
            result = subprocess.run([self.analysis_tools['readelf'], '-d', str(self.library_path)], 
                                  capture_output=True, text=True)
            
            for line in result.stdout.split('\n'):
                if 'NEEDED' in line:
                    match = re.search(r'Shared library: \[(.+?)\]', line)
                    if match:
                        lib_name = match.group(1)
                        dependencies.append({
                            "Name": lib_name,
                            "Type": "SharedLibrary",
                            "Required": True
                        })
        
        except Exception as e:
            print(f"Error extracting dependencies with readelf: {e}")
        
        return dependencies
    
    def _extract_dependencies_otool(self) -> List[Dict]:
        """Extract dependencies using otool."""
        dependencies = []
        
        try:
            result = subprocess.run([self.analysis_tools['otool'], '-L', str(self.library_path)], 
                                  capture_output=True, text=True)
            
            for line in result.stdout.split('\n'):
                line = line.strip()
                if line and not line.startswith(str(self.library_path.name)):
                    # Parse dependency line
                    match = re.match(r'(.+?)\s+\(compatibility version (.+?), current version (.+?)\)', line)
                    if match:
                        lib_path, compat_version, current_version = match.groups()
                        
                        dependencies.append({
                            "Name": Path(lib_path).name,
                            "Path": lib_path,
                            "CompatibilityVersion": compat_version,
                            "CurrentVersion": current_version,
                            "Type": "SharedLibrary",
                            "Required": True
                        })
        
        except Exception as e:
            print(f"Error extracting dependencies with otool: {e}")
        
        return dependencies
    
    def _extract_dependencies_dumpbin(self) -> List[Dict]:
        """Extract dependencies using dumpbin."""
        dependencies = []
        
        try:
            result = subprocess.run([self.analysis_tools['dumpbin'], '/IMPORTS', str(self.library_path)], 
                                  capture_output=True, text=True)
            
            current_dll = None
            for line in result.stdout.split('\n'):
                line = line.strip()
                
                # Look for DLL names
                if line.endswith('.dll') or line.endswith('.DLL'):
                    current_dll = line
                    dependencies.append({
                        "Name": current_dll,
                        "Type": "ImportLibrary", 
                        "Required": True
                    })
        
        except Exception as e:
            print(f"Error extracting dependencies with dumpbin: {e}")
        
        return dependencies
    
    def _extract_dependencies_objdump(self) -> List[Dict]:
        """Extract dependencies using objdump."""
        dependencies = []
        
        try:
            result = subprocess.run([self.analysis_tools['objdump'], '-p', str(self.library_path)], 
                                  capture_output=True, text=True)
            
            for line in result.stdout.split('\n'):
                if 'NEEDED' in line:
                    match = re.search(r'NEEDED\s+(.+)', line)
                    if match:
                        lib_name = match.group(1).strip()
                        dependencies.append({
                            "Name": lib_name,
                            "Type": "SharedLibrary",
                            "Required": True
                        })
        
        except Exception as e:
            print(f"Error extracting dependencies with objdump: {e}")
        
        return dependencies
    
    def _extract_sections(self) -> List[Dict]:
        """Extract section information."""
        sections = []
        
        if 'objdump' in self.analysis_tools:
            try:
                result = subprocess.run([self.analysis_tools['objdump'], '-h', str(self.library_path)], 
                                      capture_output=True, text=True)
                
                for line in result.stdout.split('\n'):
                    # Parse section headers
                    match = re.match(r'\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+\d+\*\*(\d+)', line)
                    if match:
                        name, size, vma, lma, offset, align = match.groups()
                        
                        sections.append({
                            "Name": name,
                            "Size": int(size, 16),
                            "VirtualAddress": vma,
                            "LoadAddress": lma,
                            "FileOffset": offset,
                            "Alignment": int(align)
                        })
            
            except Exception as e:
                print(f"Error extracting sections: {e}")
        
        return sections
    
    def _extract_notable_strings(self) -> List[str]:
        """Extract notable strings that might indicate library purpose."""
        notable_strings = []
        
        if 'strings' in self.analysis_tools:
            try:
                result = subprocess.run([self.analysis_tools['strings'], str(self.library_path)], 
                                      capture_output=True, text=True)
                
                # Filter for interesting strings
                for line in result.stdout.split('\n'):
                    line = line.strip()
                    
                    # Look for version strings, copyright, function names, etc.
                    if (len(line) > 10 and 
                        (any(keyword in line.lower() for keyword in 
                             ['version', 'copyright', 'license', 'author', 'build']) or
                         re.match(r'^[a-zA-Z_][a-zA-Z0-9_]*$', line))):
                        notable_strings.append(line)
                
                # Limit to most relevant strings
                notable_strings = notable_strings[:50]
            
            except Exception as e:
                print(f"Error extracting strings: {e}")
        
        return notable_strings
    
    def _extract_debug_info(self) -> Dict:
        """Extract debug information if available."""
        debug_info = {"HasDebugInfo": False, "DebugFormat": "None"}
        
        # Check for debug sections
        if 'objdump' in self.analysis_tools:
            try:
                result = subprocess.run([self.analysis_tools['objdump'], '-h', str(self.library_path)], 
                                      capture_output=True, text=True)
                
                debug_sections = []
                for line in result.stdout.split('\n'):
                    if any(debug_sect in line for debug_sect in ['.debug_', '.stab', '.dwarf']):
                        debug_sections.append(line.strip())
                
                if debug_sections:
                    debug_info["HasDebugInfo"] = True
                    debug_info["DebugSections"] = debug_sections
                    
                    if any('.debug_' in sect for sect in debug_sections):
                        debug_info["DebugFormat"] = "DWARF"
                    elif any('.stab' in sect for sect in debug_sections):
                        debug_info["DebugFormat"] = "STABS"
            
            except Exception as e:
                print(f"Error checking debug info: {e}")
        
        return debug_info

class CPULibraryCatalogBuilder:
    """Main catalog builder for CPU libraries."""
    
    def __init__(self):
        self.platform_name = self._get_platform_name()
    
    def _get_platform_name(self) -> str:
        """Get standardized platform name."""
        system = platform.system().lower()
        if system == "linux":
            return "Linux"
        elif system == "windows":
            return "Windows"
        elif system == "darwin":
            return "MacOS"
        else:
            return system.title()
    
    def build_catalog_entry(self, library_path: str, library_name: str = None,
                          include_source: bool = False, extract_headers: bool = False) -> Dict:
        """Build a complete catalog entry from a CPU library."""
        
        library_file = Path(library_path)
        if library_name is None:
            library_name = library_file.stem
        
        # Analyze the library
        analyzer = CPULibraryAnalyzer(library_path)
        analysis = analyzer.analyze_library()
        
        # Read binary data
        with open(library_path, 'rb') as f:
            binary_data = f.read()
        
        # Calculate checksum
        sha256_hash = hashlib.sha256(binary_data).hexdigest()
        
        # Encode to base64
        encoded_binary = base64.b64encode(binary_data).decode('utf-8')
        
        # Convert symbols to function format
        functions = self._convert_symbols_to_functions(analysis["symbols"])
        
        # Build architecture entry
        arch_entry = {
            "Name": analysis["basic_info"]["architecture"],  # Architecture as primary name
            "HostArch": analysis["basic_info"]["architecture"],  # Same as host arch for CPU libs
            "Platforms": [self.platform_name],
            "BinaryFormat": analysis["basic_info"]["binary_format"],
            "BinaryData": encoded_binary,
            "FileSize": len(binary_data),
            "Checksum": {
                "Algorithm": "SHA256",
                "Value": sha256_hash
            },
            "TargetPlatform": f"{self.platform_name.lower()}-{analysis['basic_info']['architecture'].lower()}"
        }
        
        # Build dependencies
        dependencies = []
        for dep in analysis["dependencies"]:
            dependencies.append({
                "Name": dep["Name"],
                "Version": dep.get("CurrentVersion", ">=1.0.0"),
                "Optional": not dep.get("Required", True),
                "Description": f"{'Required' if dep.get('Required') else 'Optional'} {dep.get('Type', 'library')} dependency"
            })
        
        # Build complete library entry
        library_entry = {
            "Id": f"cpu-{library_name.lower()}",
            "Name": library_name,
            "Version": "1.0.0",
            "Description": f"CPU library: {library_name}",
            "Vendor": "Unknown",
            "License": "Unknown",
            "Categories": self._categorize_library(analysis),
            "Architectures": [arch_entry],
            "Functions": functions,
            "Dependencies": dependencies,
            "BuildInfo": self._extract_build_info(analysis),
            "Metadata": {
                "SourceFile": str(library_file),
                "Framework": "Native",
                "LibraryType": analysis["basic_info"]["library_type"],
                "TotalSymbols": len(analysis["symbols"]),
                "TotalFunctions": len([s for s in analysis["symbols"] if s.get("IsFunction")]),
                "HasDebugInfo": analysis["debug_info"]["HasDebugInfo"],
                "DebugFormat": analysis["debug_info"]["DebugFormat"],
                "NotableStrings": analysis["strings"][:10],  # Top 10 strings
                "Sections": [s["Name"] for s in analysis["sections"]],
                "AnalysisTools": list(analyzer.analysis_tools.keys())
            }
        }
        
        return library_entry
    
    def _convert_symbols_to_functions(self, symbols: List[Dict]) -> List[Dict]:
        """Convert analyzed symbols to function format."""
        functions = []
        
        for symbol in symbols:
            if not symbol.get("IsFunction", False):
                continue
            
            # Try to guess parameter information from symbol name
            params = self._guess_parameters_from_name(symbol["Name"])
            
            function_info = {
                "Name": symbol["Name"],
                "Symbol": symbol["Name"],
                "TemplateParameters": [],  # Can't easily extract from binary
                "Description": f"Function {symbol['Name']}",
                "ReturnType": "unknown",  # Can't determine from symbol table
                "Parameters": params,
                "CallingConvention": self._guess_calling_convention(symbol["Name"]),
                "ThreadSafety": "Unknown",
                "Examples": [
                    {
                        "Language": "c",
                        "Code": f"// Call {symbol['Name']} function\n{symbol['Name']}({', '.join(['arg' + str(i) for i in range(len(params))])});",
                        "Description": f"Basic usage of {symbol['Name']} function"
                    }
                ],
                "Tags": ["cpu", "native", symbol.get("Type", "function").lower()],
                "SinceVersion": "1.0.0",
                "Metadata": {
                    "Address": symbol.get("Address", "0"),
                    "Size": symbol.get("Size", "unknown"),
                    "Section": symbol.get("Section", "unknown"),
                    "Visibility": symbol.get("Visibility", "unknown"),
                    "IsGlobal": symbol.get("IsGlobal", False),
                    "Source": symbol.get("Source", "unknown"),
                    "SymbolType": symbol.get("Type", "unknown")
                }
            }
            
            # Add archive object info for static libraries
            if "ArchiveObject" in symbol:
                function_info["Metadata"]["ArchiveObject"] = symbol["ArchiveObject"]
            
            functions.append(function_info)
        
        return functions
    
    def _guess_parameters_from_name(self, symbol_name: str) -> List[Dict]:
        """Attempt to guess parameters from mangled symbol names."""
        params = []
        
        # Handle C++ mangled names
        if symbol_name.startswith("_Z"):
            # This is a C++ mangled name - basic demangling attempt
            # In practice, you'd use a proper demangler like c++filt
            try:
                # Try to use c++filt if available
                result = subprocess.run(['c++filt', symbol_name], 
                                      capture_output=True, text=True)
                if result.returncode == 0:
                    demangled = result.stdout.strip()
                    params = self._parse_demangled_signature(demangled)
            except:
                pass
        
        # For C functions or if demangling failed, make educated guesses
        if not params:
            # Common patterns in function names
            if any(keyword in symbol_name.lower() for keyword in ['init', 'create', 'new']):
                params = [{"Name": "config", "Type": "void*", "Description": "Configuration parameter", "Optional": False}]
            elif any(keyword in symbol_name.lower() for keyword in ['process', 'handle', 'execute']):
                params = [
                    {"Name": "input", "Type": "void*", "Description": "Input data", "Optional": False},
                    {"Name": "size", "Type": "size_t", "Description": "Data size", "Optional": False}
                ]
            elif any(keyword in symbol_name.lower() for keyword in ['get', 'read']):
                params = [
                    {"Name": "buffer", "Type": "void*", "Description": "Output buffer", "Optional": False},
                    {"Name": "length", "Type": "size_t", "Description": "Buffer length", "Optional": False}
                ]
            elif any(keyword in symbol_name.lower() for keyword in ['set', 'write']):
                params = [
                    {"Name": "data", "Type": "const void*", "Description": "Input data", "Optional": False},
                    {"Name": "length", "Type": "size_t", "Description": "Data length", "Optional": False}
                ]
        
        return params
    
    def _parse_demangled_signature(self, demangled: str) -> List[Dict]:
        """Parse parameters from demangled C++ function signature."""
        params = []
        
        # Extract parameter list from function signature
        match = re.search(r'\((.*?)\)', demangled)
        if match:
            param_str = match.group(1)
            if param_str and param_str != 'void':
                # Split parameters by comma, handling nested templates
                param_parts = self._smart_param_split(param_str)
                
                for i, param_type in enumerate(param_parts):
                    param_type = param_type.strip()
                    if param_type:
                        params.append({
                            "Name": f"param{i+1}",
                            "Type": param_type,
                            "Description": f"Parameter {i+1} of type {param_type}",
                            "Optional": False
                        })
        
        return params
    
    def _smart_param_split(self, param_str: str) -> List[str]:
        """Smart parameter splitting that handles templates and nested types."""
        params = []
        current_param = ""
        paren_depth = 0
        template_depth = 0
        
        for char in param_str:
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
    
    def _guess_calling_convention(self, symbol_name: str) -> str:
        """Guess calling convention based on symbol name and platform."""
        if platform.system().lower() == "windows":
            if symbol_name.startswith("@") or "@" in symbol_name:
                return "FASTCALL"
            elif symbol_name.startswith("_") and symbol_name.endswith("@"):
                return "STDCALL"
            else:
                return "CDECL"
        else:
            return "CDECL"  # Default for Unix-like systems
    
    def _categorize_library(self, analysis: Dict) -> List[str]:
        """Categorize library based on symbols and strings."""
        categories = []
        
        # Analyze symbols for patterns
        symbol_names = [s["Name"].lower() for s in analysis["symbols"]]
        strings = [s.lower() for s in analysis["strings"]]
        all_text = " ".join(symbol_names + strings)
        
        # Graphics/UI libraries
        if any(keyword in all_text for keyword in ['gl', 'opengl', 'vulkan', 'directx', 'render', 'draw', 'graphics', 'gui', 'window']):
            categories.append("Graphics")
        
        # Networking libraries
        if any(keyword in all_text for keyword in ['socket', 'tcp', 'udp', 'http', 'ssl', 'tls', 'curl', 'net', 'url']):
            categories.append("Networking")
        
        # Cryptography libraries
        if any(keyword in all_text for keyword in ['crypto', 'aes', 'rsa', 'sha', 'md5', 'encrypt', 'decrypt', 'hash', 'cipher']):
            categories.append("Cryptography")
        
        # Compression libraries
        if any(keyword in all_text for keyword in ['zip', 'gzip', 'compress', 'decompress', 'inflate', 'deflate', 'zlib']):
            categories.append("Compression")
        
        # Audio libraries
        if any(keyword in all_text for keyword in ['audio', 'sound', 'wav', 'mp3', 'ogg', 'alsa', 'pulse', 'openal']):
            categories.append("Audio")
        
        # Video libraries
        if any(keyword in all_text for keyword in ['video', 'codec', 'ffmpeg', 'h264', 'mpeg', 'av', 'media']):
            categories.append("Video")
        
        # Database libraries
        if any(keyword in all_text for keyword in ['sql', 'database', 'db', 'sqlite', 'mysql', 'postgres', 'query']):
            categories.append("Database")
        
        # Machine Learning libraries
        if any(keyword in all_text for keyword in ['tensor', 'neural', 'ml', 'ai', 'matrix', 'blas', 'lapack', 'cuda']):
            categories.append("MachineLearning")
        
        # Math/Scientific libraries
        if any(keyword in all_text for keyword in ['math', 'fft', 'linear', 'algebra', 'scientific', 'numeric']):
            categories.append("Utility")
        
        # System libraries
        if any(keyword in all_text for keyword in ['system', 'kernel', 'driver', 'pthread', 'thread', 'process']):
            categories.append("System")
        
        # Default category if none detected
        if not categories:
            categories.append("Utility")
        
        return categories
    
    def _extract_build_info(self, analysis: Dict) -> Dict:
        """Extract build information from analysis."""
        build_info = {
            "Compiler": "unknown",
            "CompilerVersion": "unknown",
            "BuildFlags": [],
            "OptimizationLevel": "unknown",
            "DebugSymbols": analysis["debug_info"]["HasDebugInfo"]
        }
        
        # Try to extract compiler info from strings
        for string in analysis["strings"]:
            string_lower = string.lower()
            
            # Look for compiler identification strings
            if 'gcc' in string_lower:
                build_info["Compiler"] = "gcc"
                # Try to extract version
                version_match = re.search(r'gcc[^\d]*(\d+\.\d+(?:\.\d+)?)', string_lower)
                if version_match:
                    build_info["CompilerVersion"] = version_match.group(1)
            elif 'clang' in string_lower:
                build_info["Compiler"] = "clang"
                version_match = re.search(r'clang[^\d]*(\d+\.\d+(?:\.\d+)?)', string_lower)
                if version_match:
                    build_info["CompilerVersion"] = version_match.group(1)
            elif 'msvc' in string_lower or 'microsoft' in string_lower:
                build_info["Compiler"] = "msvc"
            
            # Look for build flags
            if '-o' in string and ('2' in string or '3' in string):
                if 'o3' in string_lower:
                    build_info["OptimizationLevel"] = "O3"
                elif 'o2' in string_lower:
                    build_info["OptimizationLevel"] = "O2"
                elif 'o1' in string_lower:
                    build_info["OptimizationLevel"] = "O1"
        
        return build_info
    
    def build_full_catalog(self, libraries: List[Dict]) -> Dict:
        """Build complete catalog with metadata."""
        catalog = {
            "Catalog": {
                "Version": "1.0.0",
                "Created": datetime.now().isoformat(),
                "Updated": datetime.now().isoformat(),
                "Description": "CPU Library Catalog"
            },
            "Libraries": libraries
        }
        
        return catalog

def main():
    parser = argparse.ArgumentParser(description="Build JSON catalog from CPU library")
    parser.add_argument("library", help="Library file (.so, .dll, .dylib, .a, .lib)")
    parser.add_argument("-o", "--output", help="Output JSON file", 
                       default="cpu_catalog.json")
    parser.add_argument("-n", "--name", help="Library name", default=None)
    parser.add_argument("-v", "--verbose", action="store_true", 
                       help="Verbose output")
    parser.add_argument("--include-source", action="store_true",
                       help="Include source code information if available")
    parser.add_argument("--extract-headers", action="store_true",
                       help="Extract header information if available")
    
    args = parser.parse_args()
    
    if not os.path.exists(args.library):
        print(f"Error: Library file '{args.library}' not found")
        sys.exit(1)
    
    try:
        builder = CPULibraryCatalogBuilder()
        
        if args.verbose:
            print(f"Processing {args.library}...")
            print(f"Platform: {builder.platform_name}")
        
        # Build library entry
        library_entry = builder.build_catalog_entry(
            args.library, args.name, args.include_source, args.extract_headers
        )
        
        # Build full catalog
        catalog = builder.build_full_catalog([library_entry])
        
        # Write to file
        with open(args.output, 'w', encoding='utf-8') as f:
            json.dump(catalog, f, indent=2)
        
        if args.verbose:
            print(f"Catalog written to {args.output}")
            print(f"Found {len(library_entry['Functions'])} functions:")
            
            # Group by symbol type
            symbol_types = {}
            for func in library_entry['Functions']:
                symbol_type = func.get('Metadata', {}).get('SymbolType', 'Function')
                if symbol_type not in symbol_types:
                    symbol_types[symbol_type] = []
                symbol_types[symbol_type].append(func)
            
            for symbol_type, funcs in symbol_types.items():
                print(f"  {symbol_type} ({len(funcs)}):")
                for func in funcs[:5]:  # Show first 5 of each type
                    print(f"    - {func['Name']}")
                    metadata = func.get('Metadata', {})
                    if metadata.get('Address') != '0':
                        print(f"      Address: {metadata.get('Address')}")
                    if metadata.get('Size') != 'unknown':
                        print(f"      Size: {metadata.get('Size')}")
                    if metadata.get('Section') != 'unknown':
                        print(f"      Section: {metadata.get('Section')}")
                if len(funcs) > 5:
                    print(f"    ... and {len(funcs) - 5} more")
            
            print(f"Built for {len(library_entry['Architectures'])} architecture(s)")
            
            # Show library categorization
            print(f"\nLibrary Categories: {', '.join(library_entry['Categories'])}")
            
            # Show dependencies
            if library_entry['Dependencies']:
                print(f"\nDependencies ({len(library_entry['Dependencies'])}):")
                for dep in library_entry['Dependencies'][:10]:  # Show first 10
                    req_str = "required" if not dep.get('Optional') else "optional"
                    print(f"  - {dep['Name']} ({req_str})")
            
            # Show metadata
            metadata = library_entry['Metadata']
            print(f"\nLibrary Analysis:")
            print(f"  Library Type: {metadata.get('LibraryType')}")
            print(f"  Total Symbols: {metadata.get('TotalSymbols')}")
            print(f"  Total Functions: {metadata.get('TotalFunctions')}")
            print(f"  Has Debug Info: {metadata.get('HasDebugInfo')}")
            print(f"  Debug Format: {metadata.get('DebugFormat')}")
            print(f"  Analysis Tools: {', '.join(metadata.get('AnalysisTools', []))}")
            
            # Show sections
            if metadata.get('Sections'):
                print(f"  Sections: {', '.join(metadata['Sections'][:10])}")
                if len(metadata['Sections']) > 10:
                    print(f"    ... and {len(metadata['Sections']) - 10} more")
    
    except Exception as e:
        print(f"Error: {e}")
        if args.verbose:
            import traceback
            traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()
