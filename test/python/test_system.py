#!/usr/bin/env python3
"""
Unit tests for Nexus System class and basic framework functionality.
"""

import unittest
import numpy as np
import sys
import os

# Add the project root to the path to import nexus
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'python'))

try:
    import nexus
except ImportError:
    print("Warning: nexus module not found. Tests will be skipped.")
    nexus = None

def get_first_runtime_with_device():
    runtimes = nexus.get_runtimes()
    for runtime in runtimes:
        devices = runtime.get_devices()
        if devices:
            return runtime, devices[0]
    return None, None

@unittest.skipIf(nexus is None, "nexus module not available")
class TestSystem(unittest.TestCase):
    """Test cases for the Nexus API entry point and runtimes."""

    def setUp(self):
        """Set up test fixtures."""
        self.runtimes = nexus.get_runtimes()
        self.runtime, self.device = get_first_runtime_with_device()

    def test_get_runtimes(self):
        """Test getting available runtimes."""
        self.assertIsInstance(self.runtimes, list)
        # Should have at least one runtime (CPU fallback)
        self.assertGreater(len(self.runtimes), 0)

    def test_get_runtime_by_index(self):
        """Test getting runtime by index."""
        if len(self.runtimes) > 0:
            runtime = self.runtimes[0]
            self.assertIsNotNone(runtime)
            self.assertTrue(hasattr(runtime, 'get_devices'))

    def test_get_runtime_invalid_index(self):
        """Test getting runtime with invalid index."""
        # Should handle invalid index gracefully
        try:
            runtime = self.runtimes[999]
            # If it doesn't raise an exception, it should return None or empty
            if runtime is not None:
                self.assertTrue(hasattr(runtime, 'get_devices'))
        except (IndexError, ValueError):
            # Expected behavior for invalid index
            pass

    def test_create_buffer(self):
        """Test buffer creation (if supported by runtime/device)."""
        if self.device is not None:
            size = 1024
            buffer = self.device.create_buffer(size)
            self.assertIsNotNone(buffer)
            self.assertEqual(buffer.get_size(), size)

    def test_create_buffer_with_data(self):
        """Test buffer creation with initial data."""
        data = np.ones(256, dtype=np.float32)
        if self.device is not None:
            buffer = self.device.create_buffer(data)
            self.assertIsNotNone(buffer)
            self.assertEqual(buffer.get_size(), data.nbytes)

    def test_copy_buffer(self):
        """Test buffer copying between devices (if supported)."""
        data = np.ones(128, dtype=np.float32)
        if self.device is not None:
            buffer = self.device.create_buffer(data)
            copied_buffer = self.device.copy_buffer(buffer)
            self.assertIsNotNone(copied_buffer)
            self.assertEqual(copied_buffer.get_size(), buffer.get_size())


@unittest.skipIf(nexus is None, "nexus module not available")
class TestFrameworkInitialization(unittest.TestCase):
    """Test cases for framework initialization and basic functionality."""

    def test_nexus_import(self):
        """Test that nexus module can be imported."""
        self.assertIsNotNone(nexus)
        self.assertTrue(hasattr(nexus, 'get_runtimes'))

    def test_basic_functionality(self):
        """Test basic framework functionality."""
        runtimes = nexus.get_runtimes()
        self.assertIsInstance(runtimes, list)
        runtime, device = get_first_runtime_with_device()
        if runtime is not None:
            devices = runtime.get_devices()
            self.assertIsInstance(devices, list)

    def test_error_handling(self):
        """Test error handling for invalid operations."""
        runtimes = nexus.get_runtimes()
        # Test accessing invalid runtime index
        try:
            runtime = runtimes[-999]
            if runtime is not None:
                self.assertTrue(hasattr(runtime, 'get_devices'))
        except (IndexError, ValueError):
            pass


if __name__ == '__main__':
    unittest.main() 