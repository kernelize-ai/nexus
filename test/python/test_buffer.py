#!/usr/bin/env python3
"""
Unit tests for Nexus Buffer class and memory operations.
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
class TestBuffer(unittest.TestCase):
    """Test cases for the Buffer class."""

    def setUp(self):
        """Set up test fixtures."""
        self.runtimes = nexus.get_runtimes()
        self.runtime, self.device = get_first_runtime_with_device()

    def test_buffer_creation_system(self):
        """Test buffer creation through system."""
        size = 1024
        buffer = self.runtime.create_buffer(size)
        self.assertIsNotNone(buffer)
        self.assertEqual(buffer.get_size(), size)

    def test_buffer_creation_device(self):
        """Test buffer creation through device."""
        if self.device is not None:
            size = 512
            buffer = self.device.create_buffer(size)
            self.assertIsNotNone(buffer)
            self.assertEqual(buffer.get_size(), size)

    def test_buffer_creation_with_data(self):
        """Test buffer creation with initial data."""
        data = np.ones(256, dtype=np.float32)
        if self.device is not None:
            buffer = self.device.create_buffer(data)
            self.assertIsNotNone(buffer)
            self.assertEqual(buffer.get_size(), data.nbytes)

    def test_buffer_creation_with_numpy_array(self):
        """Test buffer creation with numpy array."""
        if self.device is not None:
            data = np.random.rand(128).astype(np.float32)
            buffer = self.device.create_buffer(data)
            self.assertIsNotNone(buffer)
            self.assertEqual(buffer.get_size(), data.nbytes)

    def test_buffer_get_data(self):
        """Test getting buffer data."""
        if self.device is not None:
            data = np.array([1.0, 2.0, 3.0, 4.0], dtype=np.float32)
            buffer = self.device.create_buffer(data)
            buffer_data = buffer.get_data()
            self.assertIsNotNone(buffer_data)
            self.assertIsInstance(buffer_data, (bytes, memoryview))

    def test_buffer_copy_to_host(self):
        """Test copying buffer data to host."""
        if self.device is not None:
            original_data = np.random.rand(64).astype(np.float32)
            buffer = self.device.create_buffer(original_data)
            host_buffer = np.zeros_like(original_data)
            result = buffer.copy(host_buffer)
            self.assertGreaterEqual(result, 0)
            if result == 0:
                np.testing.assert_array_equal(host_buffer, original_data)

    def test_buffer_properties(self):
        """Test buffer property access."""
        if self.device is not None:
            buffer = self.device.create_buffer(256)
            try:
                prop = buffer.get_property(0)  # Some property ID
                if prop is not None:
                    self.assertTrue(hasattr(prop, 'get_value'))
            except (AttributeError, ValueError):
                pass

    def test_buffer_size_edge_cases(self):
        """Test buffer creation with edge case sizes."""
        if self.device is not None:
            try:
                buffer = self.device.create_buffer(0)
                if buffer is not None:
                    self.assertEqual(buffer.get_size(), 0)
            except (ValueError, RuntimeError):
                pass
            large_size = 1024 * 1024  # 1MB
            try:
                buffer = self.device.create_buffer(large_size)
                if buffer is not None:
                    self.assertEqual(buffer.get_size(), large_size)
            except (MemoryError, RuntimeError):
                pass

    def test_buffer_different_data_types(self):
        """Test buffer creation with different data types."""
        if self.device is not None:
            test_data = [
                np.ones(32, dtype=np.float32),
                np.ones(32, dtype=np.float64),
                np.ones(32, dtype=np.int32),
                np.ones(32, dtype=np.int64),
                np.ones(32, dtype=np.uint8),
            ]
            for data in test_data:
                buffer = self.device.create_buffer(data)
                self.assertIsNotNone(buffer)
                self.assertEqual(buffer.get_size(), data.nbytes)

    def test_buffer_copy_between_devices(self):
        """Test copying buffers between devices."""
        runtimes = nexus.get_runtimes()
        runtime1, device1 = None, None
        runtime2, device2 = None, None
        found = 0
        for runtime in runtimes:
            devices = runtime.get_devices()
            if devices:
                if found == 0:
                    runtime1, device1 = runtime, devices[0]
                    found += 1
                elif found == 1:
                    runtime2, device2 = runtime, devices[0]
                    break
        if device1 and device2:
            data = np.ones(128, dtype=np.float32)
            buffer1 = device1.create_buffer(data)
            buffer2 = device2.create_buffer(data)
            self.assertIsNotNone(buffer2)
            self.assertEqual(buffer2.get_size(), buffer1.get_size())


@unittest.skipIf(nexus is None, "nexus module not available")
class TestBufferOperations(unittest.TestCase):
    """Test cases for buffer operations and memory management."""

    def setUp(self):
        """Set up test fixtures."""
        self.runtimes = nexus.get_runtimes()
        self.runtime, self.device = get_first_runtime_with_device()

    def test_multiple_buffers(self):
        """Test creating and managing multiple buffers."""
        if self.device is not None:
            buffers = []
            sizes = [64, 128, 256, 512]
            for size in sizes:
                buffer = self.device.create_buffer(size)
                self.assertIsNotNone(buffer)
                self.assertEqual(buffer.get_size(), size)
                buffers.append(buffer)
            self.assertEqual(len(buffers), len(sizes))
            for buffer in buffers:
                self.assertIsNotNone(buffer)

    def test_buffer_lifetime(self):
        """Test buffer lifetime and cleanup."""
        if self.device is not None:
            def create_temp_buffer():
                return self.device.create_buffer(1024)
            buffer = create_temp_buffer()
            self.assertIsNotNone(buffer)
            self.assertEqual(buffer.get_size(), 1024)

    def test_buffer_error_handling(self):
        """Test error handling for invalid buffer operations."""
        if self.device is not None:
            try:
                buffer = self.device.create_buffer(-1)
                if buffer is not None:
                    self.assertGreater(buffer.get_size(), 0)
            except (ValueError, RuntimeError):
                pass
            try:
                buffer = self.device.create_buffer(1024, "invalid_data")
                if buffer is not None:
                    self.assertEqual(buffer.get_size(), 1024)
            except (TypeError, ValueError):
                pass


if __name__ == '__main__':
    unittest.main() 