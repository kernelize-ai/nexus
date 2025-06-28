#!/usr/bin/env python3
"""
Test runner for Nexus Python unit tests.
"""

import unittest
import sys
import os
import argparse

# Add the project root to the path to import nexus
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'python'))

def run_tests(test_pattern=None, verbose=False):
    """Run all tests or tests matching a pattern."""
    
    # Discover and load all test modules
    test_loader = unittest.TestLoader()
    test_suite = unittest.TestSuite()
    
    # Test modules to run
    test_modules = [
        'test_system',
        'test_runtime', 
        'test_buffer',
        'test_schedule',
        'test_properties',
        'test_integration'
    ]
    
    for module_name in test_modules:
        try:
            # Import the test module
            module = __import__(module_name)
            
            # Load tests from the module
            if test_pattern:
                # Load tests matching the pattern
                tests = test_loader.loadTestsFromName(f"{module_name}.{test_pattern}")
            else:
                # Load all tests from the module
                tests = test_loader.loadTestsFromModule(module)
            
            test_suite.addTests(tests)
            
        except ImportError as e:
            print(f"Warning: Could not import {module_name}: {e}")
        except Exception as e:
            print(f"Error loading tests from {module_name}: {e}")
    
    # Run the tests
    verbosity = 2 if verbose else 1
    runner = unittest.TextTestRunner(verbosity=verbosity)
    result = runner.run(test_suite)
    
    return result.wasSuccessful()

def main():
    """Main function to run tests."""
    parser = argparse.ArgumentParser(description='Run Nexus Python unit tests')
    parser.add_argument('--pattern', '-p', 
                       help='Test pattern to run (e.g., TestSystem.test_system_creation)')
    parser.add_argument('--verbose', '-v', action='store_true',
                       help='Verbose output')
    parser.add_argument('--list', '-l', action='store_true',
                       help='List all available tests')
    
    args = parser.parse_args()
    
    if args.list:
        # List all available tests
        test_loader = unittest.TestLoader()
        test_suite = unittest.TestSuite()
        
        test_modules = [
            'test_system',
            'test_runtime', 
            'test_buffer',
            'test_schedule',
            'test_properties',
            'test_integration'
        ]
        
        print("Available tests:")
        for module_name in test_modules:
            try:
                module = __import__(module_name)
                tests = test_loader.loadTestsFromModule(module)
                for test in tests:
                    if hasattr(test, '_tests'):
                        for subtest in test._tests:
                            if hasattr(subtest, '_tests'):
                                for method in subtest._tests:
                                    print(f"  {module_name}.{test.__class__.__name__}.{method._testMethodName}")
                            else:
                                print(f"  {module_name}.{test.__class__.__name__}.{subtest._testMethodName}")
                    else:
                        print(f"  {module_name}.{test._testMethodName}")
            except ImportError:
                print(f"  {module_name}: Not available")
        return
    
    # Run tests
    success = run_tests(args.pattern, args.verbose)
    
    if success:
        print("\nAll tests passed!")
        sys.exit(0)
    else:
        print("\nSome tests failed!")
        sys.exit(1)

if __name__ == '__main__':
    main() 