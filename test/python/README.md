# Nexus Python Unit Tests

This directory contains comprehensive unit tests for the Nexus Python API.

## Test Structure

The test suite is organized into the following modules:

- **`test_basic.py`** - Basic tests for framework initialization and runtime discovery
- **`test_runtime.py`** - Tests for Runtime and Device classes
- **`test_buffer.py`** - Tests for Buffer class and memory operations
- **`test_schedule.py`** - Tests for Schedule and Command classes
- **`test_properties.py`** - Tests for the Properties system and device information
- **`test_integration.py`** - Integration tests for complete workflows
- **`run_tests.py`** - Test runner script

## Running Tests

### Prerequisites

1. Build the Nexus framework with Python bindings:
   ```bash
   cd /path/to/nexus
   mkdir build && cd build
   cmake .. -DNEXUS_BUILD_PYTHON_MODULE=ON
   make
   ```

2. Install required Python dependencies:
   ```bash
   pip install numpy
   ```

### Running All Tests

```bash
cd test/python
python run_tests.py
```

### Running Specific Tests

```bash
# Run tests with verbose output
python run_tests.py --verbose

# Run a specific test class
python run_tests.py --pattern TestBasic

# Run a specific test method
python run_tests.py --pattern TestBasic.test_runtime_discovery

# List all available tests
python run_tests.py --list
```

### Running Individual Test Modules

```bash
# Run basic tests
python -m unittest test_basic

# Run buffer tests
python -m unittest test_buffer

# Run with verbose output
python -m unittest test_basic -v
```

## Test Features

### Graceful Degradation

The tests are designed to work even when the Nexus framework is not fully implemented:

- Tests are skipped if the `nexus` module is not available
- Tests handle missing functionality gracefully
- Error conditions are tested and expected

### Comprehensive Coverage

The test suite covers:

- **Basic Functionality**: Runtime discovery, device enumeration, framework initialization
- **Memory Management**: Buffer creation, copying, and cleanup
- **Kernel Execution**: Schedule and command creation, argument setting
- **Properties System**: Device information, property access
- **Error Handling**: Invalid operations, resource limits
- **Integration**: Complete workflows, multi-device operations
- **Performance**: Large buffers, many operations

### Platform Independence

Tests work across different platforms:

- **Linux**: CUDA, HIP, CPU backends
- **macOS**: Metal, CPU backends
- **Windows**: CPU backend

## Test Categories

### Unit Tests

- **Basic Tests**: Framework initialization and runtime discovery
- **Runtime Tests**: Runtime and device management
- **Buffer Tests**: Memory buffer operations
- **Schedule Tests**: Command scheduling and execution
- **Properties Tests**: Device information and properties

### Integration Tests

- **Workflow Tests**: Complete vector addition and computation workflows
- **Multi-device Tests**: Operations across multiple devices
- **Memory Management**: Resource allocation and cleanup
- **Error Handling**: Invalid operations and edge cases
- **Performance Tests**: Large-scale operations and stress testing

## API Structure

The Nexus Python API follows this structure:

```python
import nexus

# Get available runtimes
runtimes = nexus.get_runtimes()

# Get devices from a runtime
for runtime in runtimes:
    devices = runtime.get_devices()
    for device in devices:
        # Work with device
        buffer = device.create_buffer(data)
        schedule = device.create_schedule()
```

### Helper Functions

Tests use a helper function to find the first runtime with a valid device:

```python
def get_first_runtime_with_device():
    """Helper function to get the first runtime that has at least one device."""
    runtimes = nexus.get_runtimes()
    for runtime in runtimes:
        devices = runtime.get_devices()
        if len(devices) > 0:
            return runtime, devices[0]
    return None, None
```

## Expected Behavior

### When Nexus is Available

- All tests should run and pass
- Kernel execution tests may fail if no kernel libraries are available
- Performance tests may be limited by available memory

### When Nexus is Not Available

- Tests are skipped with appropriate warnings
- No errors are thrown
- Test runner reports skipped tests

### When Partial Implementation

- Tests for implemented features pass
- Tests for missing features are skipped or handle errors gracefully
- Property system tests may be limited if not fully implemented

## Adding New Tests

To add new tests:

1. Create a new test file following the naming convention `test_*.py`
2. Use the `@unittest.skipIf(nexus is None, "nexus module not available")` decorator
3. Add proper error handling for missing functionality
4. Update `run_tests.py` to include the new test module

Example test structure:

```python
#!/usr/bin/env python3
"""
Unit tests for new feature.
"""

import unittest
import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'python'))

try:
    import nexus
except ImportError:
    print("Warning: nexus module not found. Tests will be skipped.")
    nexus = None

def get_first_runtime_with_device():
    """Helper function to get the first runtime that has at least one device."""
    runtimes = nexus.get_runtimes()
    for runtime in runtimes:
        devices = runtime.get_devices()
        if len(devices) > 0:
            return runtime, devices[0]
    return None, None

@unittest.skipIf(nexus is None, "nexus module not available")
class TestNewFeature(unittest.TestCase):
    """Test cases for new feature."""
    
    def setUp(self):
        """Set up test fixtures."""
        self.runtime, self.device = get_first_runtime_with_device()
        self.runtimes = nexus.get_runtimes()
    
    def test_new_feature(self):
        """Test new feature functionality."""
        if self.device is not None:
            # Test implementation
            pass

if __name__ == '__main__':
    unittest.main()
```

## Troubleshooting

### Import Errors

If you get import errors:

1. Ensure Nexus is built with Python bindings
2. Check that the Python path includes the Nexus module
3. Verify that `libnexus.so` (or equivalent) is in the Python module directory

### Missing Dependencies

If numpy is not available:

```bash
pip install numpy
```

### API Changes

The tests have been updated to use the current Nexus API:

- Use `nexus.get_runtimes()` instead of `nexus.get_system()`
- Access devices through runtime objects
- Use the helper function `get_first_runtime_with_device()` for consistent device access

## Test Results

When running tests, you may see:

- **PASS**: Test completed successfully
- **SKIP**: Test skipped due to missing dependencies or functionality
- **FAIL**: Test failed due to implementation issues
- **ERROR**: Test encountered an unexpected error

Tests are designed to be informative about what functionality is available and what might be missing in the current implementation. 