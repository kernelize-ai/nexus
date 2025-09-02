#!/usr/bin/env python3
"""
Symbol Finder Tool

A tool to find matching mangled names in object files using nm and objdump with --demangle.
This tool can search for C++ symbols based on function name, template parameters, and function parameters.

Usage:
    python symbol_finder.py <object_file> --function "function_name" --params "param1_type" "param2_type"
    python symbol_finder.py <object_file> --template "function_name" "template_arg" --params "param1_type"
    python symbol_finder.py <object_file> --demangle-all
    python symbol_finder.py <object_file> --search "partial_name"
"""

import os
import sys
import re
import argparse
import subprocess
import tempfile
from pathlib import Path
from typing import List, Dict, Optional, Tuple, Set
import json

class SymbolFinder:
    """Tool to find and analyze C++ symbols in object files."""
    
    def __init__(self, object_file: str):
        self.object_file = Path(object_file)
        if not self.object_file.exists():
            raise FileNotFoundError(f"Object file not found: {object_file}")
        
        # Check if required tools are available
        self._check_tools()
        
        # Cache for symbol data
        self._symbols_cache = None
        self._demangled_cache = {}
    
    def _check_tools(self):
        """Check if required tools (nm, objdump) are available."""
        required_tools = ['nm', 'objdump']
        missing_tools = []
        
        for tool in required_tools:
            try:
                subprocess.run([tool, '--version'], capture_output=True, check=True)
            except (subprocess.CalledProcessError, FileNotFoundError):
                missing_tools.append(tool)
        
        if missing_tools:
            raise RuntimeError(f"Missing required tools: {', '.join(missing_tools)}")
    
    def get_symbols(self, force_refresh: bool = False) -> List[Dict]:
        """Get all symbols from the object file using nm."""
        if self._symbols_cache is not None and not force_refresh:
            return self._symbols_cache
        
        try:
            # First, get mangled symbols
            cmd_mangled = [
                'nm', 
                '--format=posix',  # Use POSIX format for easier parsing
                '--defined-only',  # Only show defined symbols
                str(self.object_file)
            ]
            
            result_mangled = subprocess.run(cmd_mangled, capture_output=True, text=True, check=True)
            
            # Then, get demangled symbols for comparison
            cmd_demangled = [
                'nm', 
                '--demangle',  # Demangle names
                '--format=posix',  # Use POSIX format for easier parsing
                '--defined-only',  # Only show defined symbols
                str(self.object_file)
            ]
            
            result_demangled = subprocess.run(cmd_demangled, capture_output=True, text=True, check=True)
            
            # Parse mangled output
            mangled_lines = {}
            for line in result_mangled.stdout.splitlines():
                if line.strip():
                    parts = line.split()
                    if len(parts) >= 4:
                        mangled_name = parts[0]
                        mangled_lines[mangled_name] = {
                            'type': parts[1],
                            'value': parts[2],
                            'size': parts[3] if len(parts) > 3 else '0',
                        }
            
            # Parse demangled output and match with mangled names
            symbols = []
            demangled_lines = []
            for line in result_demangled.stdout.splitlines():
                if line.strip():
                    # Handle POSIX format: name type value size
                    # The demangled name might contain spaces, so we need to be careful
                    parts = line.split()
                    if len(parts) >= 4:
                        # The last 3 parts are type, value, size
                        size = parts[-1]
                        value = parts[-2]
                        symbol_type = parts[-3]
                        # Everything else is the demangled name
                        demangled_name = ' '.join(parts[:-3])
                        
                        demangled_lines.append({
                            'name': demangled_name,
                            'type': symbol_type,
                            'value': value,
                            'size': size
                        })
            
            # Match mangled and demangled symbols by position and characteristics
            mangled_list = list(mangled_lines.items())
            
            for i, demangled in enumerate(demangled_lines):
                if i < len(mangled_list):
                    mangled_name, mangled_info = mangled_list[i]
                    symbol = {
                        'name': demangled['name'],
                        'type': demangled['type'],
                        'value': demangled['value'],
                        'size': demangled['size'],
                        'mangled_name': mangled_name
                    }
                    symbols.append(symbol)
            
            self._symbols_cache = symbols
            return symbols
            
        except subprocess.CalledProcessError as e:
            raise RuntimeError(f"Failed to extract symbols: {e}")
    
    def demangle_symbol(self, mangled_name: str) -> str:
        """Demangle a single symbol using objdump with --demangle."""
        if mangled_name in self._demangled_cache:
            return self._demangled_cache[mangled_name]
        
        try:
            # Use objdump with --demangle to demangle a single symbol
            result = subprocess.run(
                ['objdump', '-t', '-C', str(self.object_file)], 
                capture_output=True, 
                text=True, 
                check=True
            )
            
            # Find the specific symbol in the output
            for line in result.stdout.splitlines():
                if mangled_name in line:
                    # Parse the line to extract demangled name
                    parts = line.split()
                    if len(parts) >= 6:
                        # The demangled name is typically the last part
                        demangled_name = parts[-1]
                        if demangled_name != mangled_name:  # Only cache if it's actually demangled
                            self._demangled_cache[mangled_name] = demangled_name
                            return demangled_name
            
            # If not found, return the original mangled name
            return mangled_name
            
        except subprocess.CalledProcessError:
            return mangled_name  # Return original if demangling fails
    
    def find_function_by_name(self, function_name: str, exact_match: bool = True) -> List[Dict]:
        """Find functions by name (demangled)."""
        symbols = self.get_symbols()
        matches = []
        
        for symbol in symbols:
            if symbol['type'] in ['T', 't', 'W', 'w']:  # Text section symbols (functions)
                # Use the demangled name from nm --demangle
                demangled = symbol.get('name', symbol['mangled_name'])
                
                if exact_match:
                    if demangled == function_name:
                        matches.append({
                            **symbol,
                            'demangled_name': demangled
                        })
                else:
                    if function_name.lower() in demangled.lower():
                        matches.append({
                            **symbol,
                            'demangled_name': demangled
                        })
        
        return matches
    
    def find_template_function(self, function_name: str, template_params: List[str], 
                             function_params: List[str] = None) -> List[Dict]:
        """Find template function with specific template parameters."""
        symbols = self.get_symbols()
        matches = []
        
        # Build expected function signature
        expected_signature = self._build_template_signature(function_name, template_params, function_params)
        
        for symbol in symbols:
            if symbol['type'] in ['T', 't', 'W', 'w']:
                demangled = symbol.get('name', symbol['mangled_name'])
                
                # Check if this matches our expected signature
                if self._signature_matches(demangled, expected_signature):
                    matches.append({
                        **symbol,
                        'demangled_name': demangled
                    })
        
        return matches
    
    def _build_template_signature(self, function_name: str, template_params: List[str], 
                                function_params: List[str] = None) -> str:
        """Build expected function signature for matching."""
        # Basic template signature
        if template_params:
            template_part = f"{function_name}<{', '.join(template_params)}>"
        else:
            template_part = function_name
        
        # Add function parameters if provided
        if function_params:
            params_part = f"({', '.join(function_params)})"
            return f"{template_part}{params_part}"
        else:
            return template_part
    
    def _signature_matches(self, demangled: str, expected: str) -> bool:
        """Check if demangled signature matches expected signature."""
        # Normalize both signatures for comparison
        def normalize(sig):
            # Remove spaces around template brackets
            sig = re.sub(r'\s*<\s*', '<', sig)
            sig = re.sub(r'\s*>\s*', '>', sig)
            # Remove spaces around parentheses
            sig = re.sub(r'\s*\(\s*', '(', sig)
            sig = re.sub(r'\s*\)\s*', ')', sig)
            # Remove extra spaces
            sig = re.sub(r'\s+', ' ', sig).strip()
            return sig
        
        return normalize(demangled) == normalize(expected)
    
    def search_symbols(self, search_term: str, case_sensitive: bool = False) -> List[Dict]:
        """Search for symbols containing the given term."""
        symbols = self.get_symbols()
        matches = []
        
        for symbol in symbols:
            # Search in mangled name
            mangled_match = search_term in symbol['mangled_name']
            if not case_sensitive:
                mangled_match = search_term.lower() in symbol['mangled_name'].lower()
            
            # Search in demangled name
            demangled = symbol.get('name', symbol['mangled_name'])
            demangled_match = search_term in demangled
            if not case_sensitive:
                demangled_match = search_term.lower() in demangled.lower()
            
            if mangled_match or demangled_match:
                matches.append({
                    **symbol,
                    'demangled_name': demangled
                })
        
        return matches
    
    def get_detailed_symbol_info(self, symbol_name: str) -> Optional[Dict]:
        """Get detailed information about a specific symbol using objdump."""
        try:
            # Use objdump to get detailed symbol information
            cmd = [
                'objdump',
                '-t',  # Display symbol table
                '-C',  # Demangle names
                str(self.object_file)
            ]
            
            result = subprocess.run(cmd, capture_output=True, text=True, check=True)
            
            # Find the specific symbol
            for line in result.stdout.splitlines():
                if symbol_name in line:
                    # Parse objdump output
                    parts = line.split()
                    if len(parts) >= 6:
                        return {
                            'address': parts[0],
                            'flags': parts[1],
                            'section': parts[2],
                            'size': parts[3],
                            'name': parts[4],
                            'full_line': line.strip()
                        }
            
            return None
            
        except subprocess.CalledProcessError as e:
            print(f"Warning: Failed to get detailed symbol info: {e}")
            return None
    
    def export_symbols(self, output_file: str = None, format: str = 'json'):
        """Export all symbols to a file."""
        symbols = self.get_symbols()
        
        # Add demangled names (already available from nm --demangle)
        for symbol in symbols:
            symbol['demangled_name'] = symbol.get('name', symbol['mangled_name'])
        
        if output_file is None:
            output_file = f"{self.object_file.stem}_symbols.{format}"
        
        if format == 'json':
            with open(output_file, 'w') as f:
                json.dump(symbols, f, indent=2)
        elif format == 'csv':
            import csv
            with open(output_file, 'w', newline='') as f:
                writer = csv.DictWriter(f, fieldnames=['mangled_name', 'demangled_name', 'type', 'value', 'size'])
                writer.writeheader()
                writer.writerows(symbols)
        else:
            raise ValueError(f"Unsupported format: {format}")
        
        print(f"Exported {len(symbols)} symbols to {output_file}")
    
    def print_symbols(self, symbols: List[Dict], show_mangled: bool = True):
        """Print symbols in a formatted way."""
        if not symbols:
            print("No symbols found.")
            return
        
        print(f"\nFound {len(symbols)} symbol(s):")
        print("-" * 80)
        
        for i, symbol in enumerate(symbols, 1):
            # Use the demangled name from the symbol data
            demangled_name = symbol.get('name', symbol.get('demangled_name', 'N/A'))
            print(f"{i}. Function: {demangled_name}")
            if show_mangled:
                print(f"   Mangled: {symbol['mangled_name']}")
            print(f"   Type: {symbol['type']}")
            print(f"   Address: {symbol['value']}")
            print(f"   Size: {symbol['size']}")
            
            # Get detailed info if available
            detailed = self.get_detailed_symbol_info(symbol['mangled_name'])
            if detailed:
                print(f"   Section: {detailed['section']}")
                print(f"   Flags: {detailed['flags']}")
            
            print()
    
    def debug_symbol_search(self, search_term: str):
        """Debug symbol search by showing all symbols and highlighting matches."""
        symbols = self.get_symbols()
        print(f"\nDEBUG: Searching for '{search_term}' in {len(symbols)} symbols")
        print("=" * 80)
        
        matches = []
        for i, symbol in enumerate(symbols):
            demangled = symbol.get('name', symbol['mangled_name'])
            mangled = symbol['mangled_name']
            
            # Check if this symbol matches our search
            is_match = (search_term.lower() in demangled.lower() or 
                       search_term.lower() in mangled.lower())
            
            if is_match:
                matches.append(symbol)
                print(f"MATCH {len(matches)}: {demangled}")
                print(f"        Mangled: {mangled}")
                print(f"        Type: {symbol['type']}")
            else:
                # Show first few non-matches for debugging
                if i < 10:
                    print(f"      {i+1}: {demangled}")
        
        print(f"\nTotal matches found: {len(matches)}")
        return matches

def main():
    parser = argparse.ArgumentParser(description="Find C++ symbols in object files")
    parser.add_argument('object_file', help='Path to the object file')
    
    # Search options
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--function', help='Function name to search for')
    group.add_argument('--template', nargs='+', help='Template function: name template_arg1 template_arg2 ...')
    group.add_argument('--search', help='Search term for partial matching')
    group.add_argument('--demangle-all', action='store_true', help='Show all symbols with demangled names')
    
    # Additional options
    parser.add_argument('--params', nargs='+', help='Function parameters for template search')
    parser.add_argument('--exact', action='store_true', help='Use exact matching for function names')
    parser.add_argument('--case-sensitive', action='store_true', help='Case-sensitive search')
    parser.add_argument('--show-mangled', action='store_true', help='Show mangled names')
    parser.add_argument('--export', help='Export symbols to file (specify format: json or csv)')
    parser.add_argument('--debug', action='store_true', help='Enable debug mode for symbol search')
    
    args = parser.parse_args()
    
    try:
        finder = SymbolFinder(args.object_file)
        
        if args.demangle_all:
            symbols = finder.get_symbols()
            # Add demangled names
            for symbol in symbols:
                symbol['demangled_name'] = finder.demangle_symbol(symbol['mangled_name'])
            finder.print_symbols(symbols, show_mangled=args.show_mangled)
            
        elif args.function:
            symbols = finder.find_function_by_name(args.function, exact_match=args.exact)
            finder.print_symbols(symbols, show_mangled=args.show_mangled)
            
        elif args.template:
            if len(args.template) < 2:
                print("Error: Template search requires function name and at least one template argument")
                sys.exit(1)
            
            function_name = args.template[0]
            template_params = args.template[1:]
            symbols = finder.find_template_function(function_name, template_params, args.params)
            finder.print_symbols(symbols, show_mangled=args.show_mangled)
            
        elif args.search:
            if args.debug:
                symbols = finder.debug_symbol_search(args.search)
            else:
                symbols = finder.search_symbols(args.search, case_sensitive=args.case_sensitive)
            finder.print_symbols(symbols, show_mangled=args.show_mangled)
        
        # Export if requested
        if args.export:
            format_type = args.export.lower()
            if format_type not in ['json', 'csv']:
                print("Error: Export format must be 'json' or 'csv'")
                sys.exit(1)
            finder.export_symbols(format=format_type)
    
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
