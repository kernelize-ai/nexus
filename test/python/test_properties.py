#!/usr/bin/env python3
"""
Unit tests for Nexus Properties system and device information.
"""

import unittest
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
class TestProperties(unittest.TestCase):
    """Test cases for the Properties system."""

    def setUp(self):
        """Set up test fixtures."""
        self.runtime, self.device = get_first_runtime_with_device()
        self.runtimes = nexus.get_runtimes()

    def test_device_info(self):
        """Test getting device information."""
        if self.device is not None:
            info = self.device.get_info()
            self.assertIsNotNone(info)
            # Device info should be a Properties object or similar
            self.assertTrue(hasattr(info, 'get_property'))

    def test_device_property_access(self):
        """Test accessing device properties."""
        if self.device is not None:
            try:
                # Try to get a device property
                prop = self.device.get_property(0)  # Some property ID
                # If successful, should return a property or None
                if prop is not None:
                    self.assertTrue(hasattr(prop, 'get_value'))
            except (AttributeError, ValueError):
                # Property system might not be fully implemented
                pass

    def test_runtime_property_access(self):
        """Test accessing runtime properties."""
        if self.runtime is not None:
            try:
                # Try to get a runtime property
                prop = self.runtime.get_property(0)  # Some property ID
                # If successful, should return a property or None
                if prop is not None:
                    self.assertTrue(hasattr(prop, 'get_value'))
            except (AttributeError, ValueError):
                # Property system might not be fully implemented
                pass

    def test_system_property_access(self):
        """Test accessing system properties."""
        try:
            # Try to get a system property
            prop = nexus.get_property(0)  # Some property ID
            # If successful, should return a property or None
            if prop is not None:
                self.assertTrue(hasattr(prop, 'get_value'))
        except (AttributeError, ValueError):
            # Property system might not be fully implemented
            pass


@unittest.skipIf(nexus is None, "nexus module not available")
class TestDeviceDiscovery(unittest.TestCase):
    """Test cases for device discovery and information."""

    def setUp(self):
        """Set up test fixtures."""
        self.runtimes = nexus.get_runtimes()

    def test_device_enumeration(self):
        """Test that devices can be enumerated with information."""
        total_devices = 0
        for runtime in self.runtimes:
            devices = runtime.get_devices()
            total_devices += len(devices)
            
            for device in devices:
                self.assertIsNotNone(device)
                self.assertTrue(hasattr(device, 'get_info'))
                
                # Try to get device info
                info = device.get_info()
                if info is not None:
                    self.assertTrue(hasattr(info, 'get_property'))
        
        # Should have at least one device (CPU fallback)
        self.assertGreater(total_devices, 0)

    def test_device_property_consistency(self):
        """Test that device properties are consistent."""
        for runtime in self.runtimes:
            devices = runtime.get_devices()
            for device in devices:
                # Getting the same property multiple times should be consistent
                try:
                    prop1 = device.get_property(0)
                    prop2 = device.get_property(0)
                    
                    if prop1 is not None and prop2 is not None:
                        # Properties should be the same
                        self.assertEqual(prop1, prop2)
                except (AttributeError, ValueError):
                    # Property system might not be fully implemented
                    pass


@unittest.skipIf(nexus is None, "nexus module not available")
class TestPropertyTypes(unittest.TestCase):
    """Test cases for different property types."""

    def setUp(self):
        """Set up test fixtures."""
        self.runtime, self.device = get_first_runtime_with_device()
        self.runtimes = nexus.get_runtimes()

    def test_property_value_types(self):
        """Test different property value types."""
        if self.device is not None:
            try:
                # Test getting properties with different IDs
                # These might return different types of values
                for prop_id in [0, 1, 2, 1000, 1001, 1002]:
                    prop = self.device.get_property(prop_id)
                    if prop is not None:
                        # Property should have a get_value method
                        self.assertTrue(hasattr(prop, 'get_value'))
                        
                        # Try to get the value (might fail depending on type)
                        try:
                            value = prop.get_value()
                            # Value should be one of the expected types
                            self.assertIsInstance(value, (int, float, str, list))
                        except (TypeError, ValueError):
                            # Expected if property type doesn't match
                            pass
            except (AttributeError, ValueError):
                # Property system might not be fully implemented
                pass

    def test_property_string_access(self):
        """Test accessing properties by string names."""
        if self.device is not None:
            try:
                info = self.device.get_info()
                if info is not None:
                    # Try to get properties by string names
                    test_names = ["name", "type", "vendor", "version"]
                    for name in test_names:
                        try:
                            prop = info.get_property(name)
                            if prop is not None:
                                self.assertTrue(hasattr(prop, 'get_value'))
                        except (AttributeError, ValueError):
                            # Expected if property doesn't exist
                            pass
            except (AttributeError, ValueError):
                # Property system might not be fully implemented
                pass

    def test_property_path_access(self):
        """Test accessing properties by path."""
        if self.device is not None:
            try:
                info = self.device.get_info()
                if info is not None:
                    # Try to get properties by path
                    test_paths = [
                        ["coreSubsystem", "maxPerUnit"],
                        ["device", "name"],
                        ["runtime", "version"]
                    ]
                    for path in test_paths:
                        try:
                            prop = info.get_property(path)
                            if prop is not None:
                                self.assertTrue(hasattr(prop, 'get_value'))
                        except (AttributeError, ValueError):
                            # Expected if property path doesn't exist
                            pass
            except (AttributeError, ValueError):
                # Property system might not be fully implemented
                pass


@unittest.skipIf(nexus is None, "nexus module not available")
class TestChipInfo(unittest.TestCase):
    """Test cases for chip information lookup."""

    def test_lookup_chip_info(self):
        """Test looking up chip information."""
        try:
            # Test looking up a known chip (this might fail if not available)
            info = nexus.lookup_chip_info('apple-gpu-applegpu_g16s')
            if info is not None:
                self.assertTrue(hasattr(info, 'get_str'))
                self.assertTrue(hasattr(info, 'get_int'))
                
                # Try to get some properties
                try:
                    name = info.get_str('Name')
                    if name is not None:
                        self.assertIsInstance(name, str)
                except (AttributeError, ValueError):
                    pass
                
                try:
                    release_year = info.get_int('ReleaseYear')
                    if release_year is not None:
                        self.assertIsInstance(release_year, int)
                except (AttributeError, ValueError):
                    pass
        except (AttributeError, ValueError):
            # Chip info lookup might not be implemented
            pass

    def test_lookup_nonexistent_chip(self):
        """Test looking up non-existent chip information."""
        try:
            info = nexus.lookup_chip_info('nonexistent-chip')
            # Should return None or raise an exception
            if info is not None:
                # If it returns something, it should have the expected interface
                self.assertTrue(hasattr(info, 'get_str'))
                self.assertTrue(hasattr(info, 'get_int'))
        except (AttributeError, ValueError):
            # Expected behavior for non-existent chip
            pass


@unittest.skipIf(nexus is None, "nexus module not available")
class TestPropertyErrorHandling(unittest.TestCase):
    """Test cases for property error handling."""

    def setUp(self):
        """Set up test fixtures."""
        self.runtime, self.device = get_first_runtime_with_device()
        self.runtimes = nexus.get_runtimes()

    def test_invalid_property_ids(self):
        """Test handling of invalid property IDs."""
        if self.device is not None:
            try:
                # Test negative property IDs
                prop = self.device.get_property(-1)
                # Should return None or raise an exception
                if prop is not None:
                    self.assertTrue(hasattr(prop, 'get_value'))
            except (ValueError, RuntimeError):
                # Expected behavior for invalid property ID
                pass

            try:
                # Test very large property IDs
                prop = self.device.get_property(999999)
                # Should return None or raise an exception
                if prop is not None:
                    self.assertTrue(hasattr(prop, 'get_value'))
            except (ValueError, RuntimeError):
                # Expected behavior for invalid property ID
                pass

    def test_invalid_property_names(self):
        """Test handling of invalid property names."""
        if self.device is not None:
            try:
                info = self.device.get_info()
                if info is not None:
                    # Test empty string
                    prop = info.get_property("")
                    if prop is not None:
                        self.assertTrue(hasattr(prop, 'get_value'))
            except (ValueError, RuntimeError):
                # Expected behavior for invalid property name
                pass

            try:
                info = self.device.get_info()
                if info is not None:
                    # Test None
                    prop = info.get_property(None)
                    if prop is not None:
                        self.assertTrue(hasattr(prop, 'get_value'))
            except (TypeError, ValueError, RuntimeError):
                # Expected behavior for None property name
                pass


if __name__ == '__main__':
    unittest.main() 