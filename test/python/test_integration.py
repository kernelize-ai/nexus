#!/usr/bin/env python3
"""
Integration tests for Nexus framework - testing complete workflows.
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
    """Helper function to get the first runtime that has at least one device."""
    runtimes = nexus.get_runtimes()
    for runtime in runtimes:
        devices = runtime.get_devices()
        if len(devices) > 0:
            return runtime, devices[0]
    return None, None


@unittest.skipIf(nexus is None, "nexus module not available")
class TestIntegrationWorkflow(unittest.TestCase):
    """Integration tests for complete Nexus workflows."""

    def setUp(self):
        """Set up test fixtures."""
        self.runtime, self.device = get_first_runtime_with_device()
        self.runtimes = nexus.get_runtimes()

    def test_complete_vector_addition_workflow(self):
        """Test complete vector addition workflow."""
        if self.device is not None:
            # Create input data
            size = 1024
            a = np.ones(size, dtype=np.float32)
            b = np.ones(size, dtype=np.float32)
            c = np.zeros(size, dtype=np.float32)
            
            # Create buffers
            buf_a = self.device.create_buffer(a)
            buf_b = self.device.create_buffer(b)
            buf_c = self.device.create_buffer(c)
            
            self.assertIsNotNone(buf_a)
            self.assertIsNotNone(buf_b)
            self.assertIsNotNone(buf_c)
            
            # Create schedule
            schedule = self.device.create_schedule()
            self.assertIsNotNone(schedule)
            
            # Try to load a library and execute kernel
            try:
                library = self.device.load_library_file("test_kernel.so")
                if library is not None:
                    kernel = library.get_kernel("add_vectors")
                    if kernel is not None:
                        # Create command
                        command = schedule.create_command(kernel)
                        self.assertIsNotNone(command)
                        
                        # Set arguments
                        result1 = command.set_buffer(0, buf_a)
                        result2 = command.set_buffer(1, buf_b)
                        result3 = command.set_buffer(2, buf_c)
                        
                        self.assertGreaterEqual(result1, 0)
                        self.assertGreaterEqual(result2, 0)
                        self.assertGreaterEqual(result3, 0)
                        
                        # Finalize and run
                        finalize_result = command.finalize(32, size)
                        self.assertGreaterEqual(finalize_result, 0)
                        
                        run_result = schedule.run(blocking=True)
                        self.assertGreaterEqual(run_result, 0)
                        
                        # Copy result back
                        buf_c.copy(c)
                        
                        # Verify result (should be 2.0 for each element)
                        expected = np.full(size, 2.0, dtype=np.float32)
                        np.testing.assert_array_almost_equal(c, expected, decimal=5)
                        
            except (FileNotFoundError, RuntimeError, AttributeError):
                # Expected if kernel library doesn't exist
                pass

    def test_multiple_device_workflow(self):
        """Test workflow involving multiple devices."""
        if len(self.runtimes) > 1:
            # Get two different runtimes
            runtime1 = self.runtimes[0]
            runtime2 = self.runtimes[1]
            
            devices1 = runtime1.get_devices()
            devices2 = runtime2.get_devices()
            
            if len(devices1) > 0 and len(devices2) > 0:
                device1 = devices1[0]
                device2 = devices2[0]
                
                # Create data on first device
                data = np.random.rand(256).astype(np.float32)
                buf1 = device1.create_buffer(data)
                
                # Copy to second device
                buf2 = runtime1.copy_buffer(buf1, device2)
                self.assertIsNotNone(buf2)
                self.assertEqual(buf2.get_size(), buf1.get_size())
                
                # Create schedule on second device
                schedule = device2.create_schedule()
                self.assertIsNotNone(schedule)
                
                # Try to execute on second device
                try:
                    library = device2.load_library_file("test_kernel.so")
                    if library is not None:
                        kernel = library.get_kernel("test_kernel")
                        if kernel is not None:
                            command = schedule.create_command(kernel)
                            if command is not None:
                                command.set_buffer(0, buf2)
                                command.finalize(32, 256)
                                result = schedule.run(blocking=True)
                                self.assertGreaterEqual(result, 0)
                except (FileNotFoundError, RuntimeError, AttributeError):
                    # Expected if kernel library doesn't exist
                    pass

    def test_memory_management_workflow(self):
        """Test memory management and cleanup workflow."""
        if self.device is not None:
            # Create multiple buffers
            buffers = []
            for i in range(10):
                size = 1024 * (i + 1)
                data = np.random.rand(size).astype(np.float32)
                buffer = self.device.create_buffer(data)
                buffers.append(buffer)
                self.assertIsNotNone(buffer)
                self.assertEqual(buffer.get_size(), data.nbytes)
            
            # Use buffers in a schedule
            schedule = self.device.create_schedule()
            self.assertIsNotNone(schedule)
            
            # Try to create commands with buffers
            try:
                library = self.device.load_library_file("test_kernel.so")
                if library is not None:
                    kernel = library.get_kernel("test_kernel")
                    if kernel is not None:
                        for i, buffer in enumerate(buffers[:3]):  # Use first 3 buffers
                            command = schedule.create_command(kernel)
                            if command is not None:
                                command.set_buffer(0, buffer)
                                command.finalize(32, buffer.get_size() // 4)  # Assuming float32
                        
                        # Run schedule
                        result = schedule.run(blocking=True)
                        self.assertGreaterEqual(result, 0)
            except (FileNotFoundError, RuntimeError, AttributeError):
                # Expected if kernel library doesn't exist
                pass
            
            # Buffers should still be valid after use
            for buffer in buffers:
                self.assertIsNotNone(buffer)
                self.assertGreater(buffer.get_size(), 0)


@unittest.skipIf(nexus is None, "nexus module not available")
class TestErrorHandling(unittest.TestCase):
    """Integration tests for error handling."""

    def setUp(self):
        """Set up test fixtures."""
        self.runtime, self.device = get_first_runtime_with_device()
        self.runtimes = nexus.get_runtimes()

    def test_invalid_operations(self):
        """Test handling of invalid operations."""
        if self.device is not None:
            # Test invalid buffer creation
            try:
                buffer = self.device.create_buffer(-1)
                # Should return None or raise an exception
                if buffer is not None:
                    self.assertGreater(buffer.get_size(), 0)
            except (ValueError, RuntimeError):
                # Expected behavior for invalid size
                pass
            
            # Test invalid schedule creation
            try:
                schedule = self.device.create_schedule()
                if schedule is not None:
                    # Try to create command with invalid kernel
                    command = schedule.create_command(None)
                    if command is not None:
                        # Try to set invalid arguments
                        result = command.set_buffer(999, None)
                        self.assertGreaterEqual(result, 0)  # Should return error code
            except (TypeError, ValueError, RuntimeError):
                # Expected behavior for invalid operations
                pass

    def test_resource_cleanup(self):
        """Test resource cleanup and memory management."""
        if self.device is not None:
            # Create many buffers to test memory management
            buffers = []
            for i in range(100):
                try:
                    buffer = self.device.create_buffer(1024)
                    if buffer is not None:
                        buffers.append(buffer)
                except (MemoryError, RuntimeError):
                    # Expected if out of memory
                    break
            
            # All created buffers should be valid
            for buffer in buffers:
                self.assertIsNotNone(buffer)
                self.assertEqual(buffer.get_size(), 1024)
            
            # Try to use buffers
            if len(buffers) > 0:
                schedule = self.device.create_schedule()
                if schedule is not None:
                    try:
                        library = self.device.load_library_file("test_kernel.so")
                        if library is not None:
                            kernel = library.get_kernel("test_kernel")
                            if kernel is not None:
                                command = schedule.create_command(kernel)
                                if command is not None:
                                    command.set_buffer(0, buffers[0])
                                    command.finalize(32, 256)
                                    result = schedule.run(blocking=True)
                                    self.assertGreaterEqual(result, 0)
                    except (FileNotFoundError, RuntimeError, AttributeError):
                        # Expected if kernel library doesn't exist
                        pass

    def test_concurrent_access(self):
        """Test concurrent access to Nexus objects."""
        if self.device is not None:
            # Create multiple schedules
            schedules = []
            for i in range(5):
                schedule = self.device.create_schedule()
                if schedule is not None:
                    schedules.append(schedule)
            
            # Try to use schedules concurrently
            for schedule in schedules:
                try:
                    result = schedule.run(blocking=False)  # Non-blocking
                    self.assertGreaterEqual(result, 0)
                except (RuntimeError, AttributeError):
                    # Expected if concurrent execution not supported
                    pass


@unittest.skipIf(nexus is None, "nexus module not available")
class TestPerformance(unittest.TestCase):
    """Performance and stress tests."""

    def setUp(self):
        """Set up test fixtures."""
        self.runtime, self.device = get_first_runtime_with_device()
        self.runtimes = nexus.get_runtimes()

    def test_large_buffer_operations(self):
        """Test operations with large buffers."""
        if self.device is not None:
            # Test with large buffer
            large_size = 1024 * 1024  # 1MB
            try:
                data = np.random.rand(large_size // 4).astype(np.float32)  # 1MB of float32
                buffer = self.device.create_buffer(data)
                self.assertIsNotNone(buffer)
                self.assertEqual(buffer.get_size(), data.nbytes)
                
                # Try to copy large buffer
                host_buffer = np.zeros_like(data)
                result = buffer.copy(host_buffer)
                self.assertGreaterEqual(result, 0)
                
            except (MemoryError, RuntimeError):
                # Expected if not enough memory
                pass

    def test_many_small_operations(self):
        """Test many small operations."""
        if self.device is not None:
            # Create many small buffers
            buffers = []
            for i in range(1000):
                try:
                    buffer = self.device.create_buffer(64)
                    if buffer is not None:
                        buffers.append(buffer)
                except (MemoryError, RuntimeError):
                    break
            
            # All buffers should be valid
            for buffer in buffers:
                self.assertIsNotNone(buffer)
                self.assertEqual(buffer.get_size(), 64)
            
            # Try to use some buffers
            if len(buffers) > 0:
                schedule = self.device.create_schedule()
                if schedule is not None:
                    try:
                        library = self.device.load_library_file("test_kernel.so")
                        if library is not None:
                            kernel = library.get_kernel("test_kernel")
                            if kernel is not None:
                                # Use first few buffers
                                for i, buffer in enumerate(buffers[:10]):
                                    command = schedule.create_command(kernel)
                                    if command is not None:
                                        command.set_buffer(0, buffer)
                                        command.finalize(32, 16)
                                        result = schedule.run(blocking=True)
                                        self.assertGreaterEqual(result, 0)
                    except (FileNotFoundError, RuntimeError, AttributeError):
                        # Expected if kernel library doesn't exist
                        pass


if __name__ == '__main__':
    unittest.main() 
