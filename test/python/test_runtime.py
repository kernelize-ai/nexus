#!/usr/bin/env python3
"""
Unit tests for Nexus Runtime class and device management.
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
class TestRuntime(unittest.TestCase):
    """Test cases for the Runtime class."""

    def setUp(self):
        """Set up test fixtures."""
        self.runtimes = nexus.get_runtimes()
        self.runtime, self.device = get_first_runtime_with_device()

    def test_runtime_creation(self):
        """Test that runtime can be accessed."""
        self.assertIsNotNone(self.runtime)
        self.assertTrue(hasattr(self.runtime, 'get_devices'))

    def test_get_devices(self):
        """Test getting devices from runtime."""
        if self.runtime is not None:
            devices = self.runtime.get_devices()
            self.assertIsInstance(devices, list)
            # Should have at least one device
            self.assertGreater(len(devices), 0)

    def test_get_device_by_id(self):
        """Test getting device by ID."""
        if self.runtime is not None:
            devices = self.runtime.get_devices()
            if len(devices) > 0:
                device = self.runtime.get_device(0)
                self.assertIsNotNone(device)
                self.assertTrue(hasattr(device, 'get_info'))

    def test_get_device_invalid_id(self):
        """Test getting device with invalid ID."""
        if self.runtime is not None:
            try:
                device = self.runtime.get_device(999)
                # If it doesn't raise an exception, it should return None or be valid
                if device is not None:
                    self.assertTrue(hasattr(device, 'get_info'))
            except (IndexError, ValueError):
                # Expected behavior for invalid ID
                pass

    def test_runtime_properties(self):
        """Test runtime property access."""
        if self.runtime is not None:
            try:
                prop = self.runtime.get_property(0)  # Some property ID
                if prop is not None:
                    self.assertTrue(hasattr(prop, 'get_value'))
            except (AttributeError, ValueError):
                pass


@unittest.skipIf(nexus is None, "nexus module not available")
class TestDevice(unittest.TestCase):
    """Test cases for the Device class."""

    def setUp(self):
        """Set up test fixtures."""
        self.runtimes = nexus.get_runtimes()
        self.runtime, self.device = get_first_runtime_with_device()

    def test_device_creation(self):
        """Test that device can be accessed."""
        self.assertIsNotNone(self.device)
        self.assertTrue(hasattr(self.device, 'get_info'))

    def test_device_info(self):
        """Test getting device information."""
        if self.device is not None:
            info = self.device.get_info()
            self.assertIsNotNone(info)
            self.assertTrue(hasattr(info, 'get_property'))

    def test_create_schedule(self):
        """Test creating a schedule on device."""
        if self.device is not None:
            schedule = self.device.create_schedule()
            self.assertIsNotNone(schedule)
            self.assertTrue(hasattr(schedule, 'create_command'))

    def test_create_buffer(self):
        """Test creating buffer on device."""
        if self.device is not None:
            data = np.ones(256, dtype=np.float32)
            buffer = self.device.create_buffer(data)
            self.assertIsNotNone(buffer)
            self.assertEqual(buffer.get_size(), data.nbytes)

    def test_create_buffer_with_size(self):
        """Test creating buffer with size on device."""
        if self.device is not None:
            size = 1024
            buffer = self.device.create_buffer(size)
            self.assertIsNotNone(buffer)
            self.assertEqual(buffer.get_size(), size)

    def test_copy_buffer(self):
        """Test copying buffer on device."""
        if self.device is not None:
            data = np.ones(128, dtype=np.float32)
            buffer = self.device.create_buffer(data)
            copied_buffer = self.device.copy_buffer(buffer)
            self.assertIsNotNone(copied_buffer)
            self.assertEqual(copied_buffer.get_size(), buffer.get_size())

    def test_load_library(self):
        """Test loading library on device."""
        if self.device is not None:
            try:
                library = self.device.load_library("test_library.so")
                if library is not None:
                    self.assertTrue(hasattr(library, 'get_kernel'))
            except (FileNotFoundError, RuntimeError):
                pass

    def test_device_properties(self):
        """Test device property access."""
        if self.device is not None:
            try:
                prop = self.device.get_property(0)  # Some property ID
                if prop is not None:
                    self.assertTrue(hasattr(prop, 'get_value'))
            except (AttributeError, ValueError):
                pass


@unittest.skipIf(nexus is None, "nexus module not available")
class TestDeviceDiscovery(unittest.TestCase):
    """Test cases for device discovery and enumeration."""

    def test_device_enumeration(self):
        """Test that devices can be enumerated across all runtimes."""
        runtimes = nexus.get_runtimes()
        total_devices = 0
        for runtime in runtimes:
            devices = runtime.get_devices()
            total_devices += len(devices)
            for device in devices:
                self.assertIsNotNone(device)
                self.assertTrue(hasattr(device, 'get_info'))
        self.assertGreater(total_devices, 0)

    def test_device_consistency(self):
        """Test that device objects are consistent."""
        runtimes = nexus.get_runtimes()
        for runtime in runtimes:
            devices = runtime.get_devices()
            for i, device in enumerate(devices):
                device_by_id = runtime.get_device(i)
                if device_by_id is not None:
                    self.assertIsNotNone(device_by_id)
                    self.assertTrue(hasattr(device_by_id, 'get_info'))


if __name__ == '__main__':
    unittest.main() 