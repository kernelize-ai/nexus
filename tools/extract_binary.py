#!/usr/bin/env python3
"""
Binary Extractor Tool

Extracts binary data from JSON catalogs back into files with the corresponding type.
This tool can reconstruct compiled libraries from the binary data stored in the catalog.
"""

import argparse
import base64
import hashlib
import json
import sys
from pathlib import Path
from typing import Dict, Any, Optional, List


class BinaryExtractor:
    """Extract binary data from JSON catalogs back into files"""
    
    def __init__(self):
        self.supported_formats = {
            'SO': '.so',      # Linux shared library
            'DYLIB': '.dylib', # macOS shared library
            'DLL': '.dll',     # Windows shared library
            'A': '.a',         # Static archive
            'LIB': '.lib',     # Windows static library
            'EXE': '.exe',     # Windows executable
            'ELF': '',         # Linux executable (no extension)
            'MACHO': '',       # macOS executable (no extension)
        }
    
    def _calculate_checksum(self, data: bytes, algorithm: str = 'SHA256') -> str:
        """Calculate checksum for binary data"""
        if algorithm.upper() == 'SHA256':
            return hashlib.sha256(data).hexdigest()
        elif algorithm.upper() == 'MD5':
            return hashlib.md5(data).hexdigest()
        elif algorithm.upper() == 'SHA1':
            return hashlib.sha1(data).hexdigest()
        else:
            raise ValueError(f"Unsupported checksum algorithm: {algorithm}")
    
    def _verify_checksum(self, data: bytes, expected_checksum: str, algorithm: str = 'SHA256') -> bool:
        """Verify checksum of binary data"""
        calculated_checksum = self._calculate_checksum(data, algorithm)
        return calculated_checksum.lower() == expected_checksum.lower()
    
    def extract_binary_from_catalog(self, catalog_file: str, 
                                   library_name: Optional[str] = None,
                                   architecture: Optional[str] = None,
                                   output_dir: Optional[str] = None,
                                   output_name: Optional[str] = None,
                                   skip_checksum: bool = False) -> List[str]:
        """
        Extract binary data from a JSON catalog file
        
        Args:
            catalog_file: Path to the JSON catalog file
            library_name: Specific library name to extract (if None, extracts all)
            architecture: Specific architecture to extract (if None, extracts all)
            output_dir: Output directory for extracted files
            output_name: Custom output filename (without extension)
            
        Returns:
            List of paths to extracted files
        """
        # Load catalog
        with open(catalog_file, 'r') as f:
            catalog = json.load(f)
        
        extracted_files = []
        
        # Process each library
        for library in catalog.get('Libraries', []):
            lib_name = library.get('Name', 'Unknown')
            
            # Skip if specific library name requested and doesn't match
            if library_name and lib_name != library_name:
                continue
            
            # Process each architecture
            for arch in library.get('Architectures', []):
                arch_name = arch.get('Name', 'Unknown')
                binary_format = arch.get('BinaryFormat', 'Unknown')
                binary_data = arch.get('BinaryData', '')
                
                # Skip if specific architecture requested and doesn't match
                if architecture and arch_name != architecture:
                    continue
                
                # Skip if no binary data
                if not binary_data:
                    print(f"Warning: No binary data found for {lib_name} ({arch_name})")
                    continue
                
                # Determine file extension
                extension = self.supported_formats.get(binary_format, '.bin')
                
                # Generate output filename
                if output_name:
                    filename = f"{output_name}{extension}"
                else:
                    # Use library name and architecture
                    safe_lib_name = lib_name.lower().replace(' ', '_').replace('-', '_')
                    safe_arch_name = arch_name.lower().replace(' ', '_')
                    filename = f"{safe_lib_name}_{safe_arch_name}{extension}"
                
                # Determine output path
                if output_dir:
                    output_path = Path(output_dir) / filename
                    output_path.parent.mkdir(parents=True, exist_ok=True)
                else:
                    output_path = Path(filename)
                
                # Extract binary data
                try:
                    # Decode base64 data
                    binary_bytes = base64.b64decode(binary_data)
                    
                    # Verify checksum if available and not skipped
                    if not skip_checksum:
                        checksum_info = arch.get('Checksum', {})
                        if checksum_info:
                            expected_checksum = checksum_info.get('Value', '')
                            algorithm = checksum_info.get('Algorithm', 'SHA256')
                            
                            if expected_checksum:
                                if self._verify_checksum(binary_bytes, expected_checksum, algorithm):
                                    print(f"✓ Checksum verified ({algorithm})")
                                else:
                                    print(f"✗ Checksum verification failed ({algorithm})")
                                    print(f"  Expected: {expected_checksum}")
                                    calculated = self._calculate_checksum(binary_bytes, algorithm)
                                    print(f"  Calculated: {calculated}")
                                    # Continue with extraction but warn user
                            else:
                                print(f"Warning: No checksum value found for {lib_name} ({arch_name})")
                    else:
                        print("⚠ Checksum verification skipped")
                    
                    # Write to file
                    with open(output_path, 'wb') as f:
                        f.write(binary_bytes)
                    
                    print(f"Extracted: {output_path} ({len(binary_bytes)} bytes)")
                    extracted_files.append(str(output_path))
                    
                except Exception as e:
                    print(f"Error extracting {lib_name} ({arch_name}): {e}")
        
        return extracted_files
    
    def list_available_binaries(self, catalog_file: str) -> None:
        """List all available binaries in the catalog"""
        with open(catalog_file, 'r') as f:
            catalog = json.load(f)
        
        print(f"Available binaries in {catalog_file}:")
        print("=" * 60)
        
        for library in catalog.get('Libraries', []):
            lib_name = library.get('Name', 'Unknown')
            lib_version = library.get('Version', 'Unknown')
            
            print(f"\nLibrary: {lib_name} v{lib_version}")
            print("-" * 40)
            
            for arch in library.get('Architectures', []):
                arch_name = arch.get('Name', 'Unknown')
                binary_format = arch.get('BinaryFormat', 'Unknown')
                file_size = arch.get('FileSize', 0)
                has_data = bool(arch.get('BinaryData', ''))
                checksum_info = arch.get('Checksum', {})
                has_checksum = bool(checksum_info.get('Value', ''))
                
                status = "✓ Available" if has_data else "✗ No data"
                checksum_status = "✓ Checksum" if has_checksum else "✗ No checksum"
                print(f"  {arch_name} ({binary_format}): {file_size} bytes - {status} - {checksum_status}")
    
    def extract_specific_binary(self, catalog_file: str, 
                               library_name: str, 
                               architecture: str,
                               output_path: Optional[str] = None,
                               skip_checksum: bool = False) -> Optional[str]:
        """
        Extract a specific binary from the catalog
        
        Args:
            catalog_file: Path to the JSON catalog file
            library_name: Name of the library to extract
            architecture: Architecture to extract
            output_path: Specific output path (optional)
            
        Returns:
            Path to extracted file or None if failed
        """
        with open(catalog_file, 'r') as f:
            catalog = json.load(f)
        
        # Find the specific library and architecture
        for library in catalog.get('Libraries', []):
            if library.get('Name') == library_name:
                for arch in library.get('Architectures', []):
                    if arch.get('Name') == architecture:
                        binary_format = arch.get('BinaryFormat', 'Unknown')
                        binary_data = arch.get('BinaryData', '')
                        
                        if not binary_data:
                            print(f"Error: No binary data found for {library_name} ({architecture})")
                            return None
                        
                        # Determine output path
                        if output_path:
                            output_file = Path(output_path)
                        else:
                            extension = self.supported_formats.get(binary_format, '.bin')
                            safe_lib_name = library_name.lower().replace(' ', '_').replace('-', '_')
                            safe_arch_name = architecture.lower().replace(' ', '_')
                            output_file = Path(f"{safe_lib_name}_{safe_arch_name}{extension}")
                        
                        # Extract binary data
                        try:
                            binary_bytes = base64.b64decode(binary_data)
                            
                            # Verify checksum if available and not skipped
                            if not skip_checksum:
                                checksum_info = arch.get('Checksum', {})
                                if checksum_info:
                                    expected_checksum = checksum_info.get('Value', '')
                                    algorithm = checksum_info.get('Algorithm', 'SHA256')
                                    
                                    if expected_checksum:
                                        if self._verify_checksum(binary_bytes, expected_checksum, algorithm):
                                            print(f"✓ Checksum verified ({algorithm})")
                                        else:
                                            print(f"✗ Checksum verification failed ({algorithm})")
                                            print(f"  Expected: {expected_checksum}")
                                            calculated = self._calculate_checksum(binary_bytes, algorithm)
                                            print(f"  Calculated: {calculated}")
                                            # Continue with extraction but warn user
                                    else:
                                        print(f"Warning: No checksum value found for {library_name} ({architecture})")
                            else:
                                print("⚠ Checksum verification skipped")
                            
                            # Create output directory if needed
                            output_file.parent.mkdir(parents=True, exist_ok=True)
                            
                            # Write to file
                            with open(output_file, 'wb') as f:
                                f.write(binary_bytes)
                            
                            print(f"Extracted: {output_file} ({len(binary_bytes)} bytes)")
                            return str(output_file)
                            
                        except Exception as e:
                            print(f"Error extracting binary: {e}")
                            return None
        
        print(f"Error: Library '{library_name}' with architecture '{architecture}' not found")
        return None


def main():
    parser = argparse.ArgumentParser(
        description='Extract binary data from JSON catalogs back into files',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # List all available binaries in a catalog
  python extract_binary.py catalog.json --list
  
  # Extract all binaries from a catalog
  python extract_binary.py catalog.json --output-dir ./extracted
  
  # Extract a specific library
  python extract_binary.py catalog.json --library "C++ Library" --output-dir ./extracted
  
  # Extract a specific library and architecture
  python extract_binary.py catalog.json --library "C++ Library" --arch "x86_64" --output-dir ./extracted
  
  # Extract with custom filename
  python extract_binary.py catalog.json --library "C++ Library" --output-name "my_library"
        """
    )
    
    parser.add_argument('catalog_file', help='Path to the JSON catalog file')
    parser.add_argument('--list', action='store_true', help='List all available binaries in the catalog')
    parser.add_argument('--library', help='Specific library name to extract')
    parser.add_argument('--arch', '--architecture', help='Specific architecture to extract')
    parser.add_argument('--output-dir', '-o', help='Output directory for extracted files')
    parser.add_argument('--output-name', help='Custom output filename (without extension)')
    parser.add_argument('--output-path', help='Specific output path for single extraction')
    parser.add_argument('--skip-checksum', action='store_true', help='Skip checksum verification')
    
    args = parser.parse_args()
    
    # Check if catalog file exists
    if not Path(args.catalog_file).exists():
        print(f"Error: Catalog file '{args.catalog_file}' not found")
        return 1
    
    extractor = BinaryExtractor()
    
    try:
        if args.list:
            # List available binaries
            extractor.list_available_binaries(args.catalog_file)
            return 0
        
        elif args.library and args.arch:
            # Extract specific library and architecture
            result = extractor.extract_specific_binary(
                args.catalog_file,
                args.library,
                args.arch,
                args.output_path,
                skip_checksum=args.skip_checksum
            )
            return 0 if result else 1
        
        else:
            # Extract binaries (all or filtered)
            extracted_files = extractor.extract_binary_from_catalog(
                args.catalog_file,
                library_name=args.library,
                architecture=args.arch,
                output_dir=args.output_dir,
                output_name=args.output_name,
                skip_checksum=args.skip_checksum
            )
            
            if extracted_files:
                print(f"\nSuccessfully extracted {len(extracted_files)} file(s)")
                return 0
            else:
                print("No files were extracted")
                return 1
    
    except Exception as e:
        print(f"Error: {e}")
        return 1


if __name__ == '__main__':
    exit(main())
