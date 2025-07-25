#!/usr/bin/env python3
"""
Unit tests for Nexus Schedule and Command classes for kernel execution.
"""

import unittest
import numpy as np
import sys
import os
import tempfile

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
class TestSchedule(unittest.TestCase):
    """Test cases for the Schedule class."""

    def setUp(self):
        """Set up test fixtures."""
        self.runtimes = nexus.get_runtimes()
        self.runtime, self.device = get_first_runtime_with_device()
        if self.device is not None:
            self.schedule = self.device.create_schedule()
        else:
            self.schedule = None

    def test_schedule_creation(self):
        """Test that schedule can be created."""
        if self.device is not None:
            self.assertIsNotNone(self.schedule)
            self.assertTrue(hasattr(self.schedule, 'create_command'))

    def test_schedule_properties(self):
        """Test schedule property access."""
        if self.schedule is not None:
            try:
                prop = self.schedule.get_property(0)  # Some property ID
                if prop is not None:
                    self.assertTrue(hasattr(prop, 'get_value'))
            except (AttributeError, ValueError):
                pass

    def test_schedule_run_empty(self):
        """Test running an empty schedule."""
        if self.schedule is not None:
            result = self.schedule.run(blocking=True)
            self.assertGreaterEqual(result, 0)

    def test_schedule_run_non_blocking(self):
        """Test running schedule in non-blocking mode."""
        if self.schedule is not None:
            result = self.schedule.run(blocking=False)
            self.assertGreaterEqual(result, 0)


@unittest.skipIf(nexus is None, "nexus module not available")
class TestCommand(unittest.TestCase):
    """Test cases for the Command class."""

    def setUp(self):
        """Set up test fixtures."""
        self.runtimes = nexus.get_runtimes()
        self.runtime, self.device = get_first_runtime_with_device()
        if self.device is not None:
            self.schedule = self.device.create_schedule()
        else:
            self.schedule = None

    def test_command_creation(self):
        """Test command creation (requires a kernel)."""
        if self.schedule is not None:
            try:
                command = self.schedule.create_command(None)
                if command is not None:
                    self.assertTrue(hasattr(command, 'set_buffer'))
                    self.assertTrue(hasattr(command, 'finalize'))
            except (TypeError, ValueError, RuntimeError):
                pass

    def test_command_properties(self):
        """Test command property access."""
        if self.schedule is not None:
            try:
                command = self.schedule.create_command(None)
                if command is not None:
                    prop = command.get_property(0)  # Some property ID
                    if prop is not None:
                        self.assertTrue(hasattr(prop, 'get_value'))
            except (TypeError, ValueError, RuntimeError, AttributeError):
                pass


@unittest.skipIf(nexus is None, "nexus module not available")
class TestKernelExecution(unittest.TestCase):
    """Test cases for kernel execution workflow."""

    def setUp(self):
        """Set up test fixtures."""
        self.runtimes = nexus.get_runtimes()
        self.runtime, self.device = get_first_runtime_with_device()

    def test_kernel_execution_workflow(self):
        """Test the complete kernel execution workflow."""
        if self.device is not None:
            data1 = np.ones(256, dtype=np.float32)
            data2 = np.ones(256, dtype=np.float32)
            result = np.zeros(256, dtype=np.float32)
            buf1 = self.device.create_buffer(data1)
            buf2 = self.device.create_buffer(data2)
            buf_result = self.device.create_buffer(result)
            self.assertIsNotNone(buf1)
            self.assertIsNotNone(buf2)
            self.assertIsNotNone(buf_result)
            schedule = self.device.create_schedule()
            self.assertIsNotNone(schedule)
            try:
                library = self.device.load_library_file("test_kernel.so")
                if library is not None:
                    kernel = library.get_kernel("test_kernel")
                    if kernel is not None:
                        command = schedule.create_command(kernel)
                        self.assertIsNotNone(command)
                        result1 = command.set_buffer(0, buf1)
                        result2 = command.set_buffer(1, buf2)
                        result3 = command.set_buffer(2, buf_result)
                        self.assertGreaterEqual(result1, 0)
                        self.assertGreaterEqual(result2, 0)
                        self.assertGreaterEqual(result3, 0)
                        finalize_result = command.finalize(32, 256)
                        self.assertGreaterEqual(finalize_result, 0)
                        run_result = schedule.run(blocking=True)
                        self.assertGreaterEqual(run_result, 0)
                        buf_result.copy(result)
            except (FileNotFoundError, RuntimeError, AttributeError):
                pass

    def test_multiple_commands(self):
        """Test creating multiple commands in a schedule."""
        if self.device is not None:
            schedule = self.device.create_schedule()
            commands = []
            try:
                for i in range(3):
                    command = schedule.create_command(None)
                    if command is not None:
                        commands.append(command)
                        self.assertTrue(hasattr(command, 'set_buffer'))
                        self.assertTrue(hasattr(command, 'finalize'))
            except (TypeError, ValueError, RuntimeError):
                pass
            result = schedule.run(blocking=True)
            self.assertGreaterEqual(result, 0)

    def test_command_argument_setting(self):
        """Test setting command arguments."""
        if self.device is not None:
            schedule = self.device.create_schedule()
            buf1 = self.device.create_buffer(256)
            buf2 = self.device.create_buffer(256)
            try:
                command = schedule.create_command(None)
                if command is not None:
                    result1 = command.set_buffer(0, buf1)
                    result2 = command.set_buffer(1, buf2)
                    self.assertGreaterEqual(result1, 0)
                    self.assertGreaterEqual(result2, 0)
                    try:
                        result3 = command.set_buffer(999, buf1)
                        if result3 is not None:
                            self.assertGreaterEqual(result3, 0)
                    except (IndexError, ValueError):
                        pass
            except (TypeError, ValueError, RuntimeError):
                pass


@unittest.skipIf(nexus is None, "nexus module not available")
class TestLibraryAndKernel(unittest.TestCase):
    """Test cases for Library and Kernel classes."""

    def setUp(self):
        """Set up test fixtures."""
        self.runtimes = nexus.get_runtimes()
        self.runtime, self.device = get_first_runtime_with_device()

    def test_library_loading(self):
        """Test library loading functionality."""
        if self.device is not None:
            try:
                library = self.device.load_library_file("non_existent_library.so")
                if library is not None:
                    self.assertTrue(hasattr(library, 'get_kernel'))
            except (FileNotFoundError, RuntimeError):
                pass

    def test_kernel_retrieval(self):
        """Test kernel retrieval from library."""
        if self.device is not None:
            try:
                library = self.device.load_library_file("test_library.so")
                if library is not None:
                    kernel = library.get_kernel("test_kernel")
                    if kernel is not None:
                        self.assertTrue(hasattr(kernel, 'get_property'))
                        try:
                            prop = kernel.get_property(0)  # Some property ID
                            if prop is not None:
                                self.assertTrue(hasattr(prop, 'get_value'))
                        except (AttributeError, ValueError):
                            pass
            except (FileNotFoundError, RuntimeError):
                pass


if __name__ == '__main__':
    unittest.main() 
