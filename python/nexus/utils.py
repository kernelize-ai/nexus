import importlib
import nexus

"""
Utility functions for the Nexus module.

This module provides helper functions and utilities for working with Nexus.
"""

numpy_loaded = False
try:
    importlib.find_loader('numpy')
    numpy_loaded = True
except ImportError:
    numpy_loaded = False

torch_loaded = False
try:
    importlib.find_loader('torch')
    torch_loaded = True
except ImportError:
    torch_loaded = False

def version_info():
    """
    Get version information about the Nexus module.
    
    Returns:
        dict: A dictionary containing version information.
    """
    import sys
    return {
        'version': __version__ if '__version__' in globals() else '0.0.1',
        'python_version': sys.version,
        'platform': sys.platform
    }


def list_available_runtimes():
    """
    Get a list of all available runtime backends.
    
    Returns:
        list: A list of runtime names.
    """
    # This will use the C++ bindings imported in __init__.py
    try:
        runtimes = get_runtimes()
        return [str(rt) for rt in runtimes] if runtimes else []
    except NameError:
        return []


def get_device_count(runtime_name=None):
    """
    Get the number of available devices.
    
    Args:
        runtime_name (str, optional): Filter by runtime name. If None, counts all devices.
        
    Returns:
        int: Number of available devices.
    """
    try:
        runtimes = get_runtimes()
        total_count = 0
        
        for rt in runtimes:
            if runtime_name is None or str(rt) == runtime_name:
                devices = rt.get_devices()
                total_count += len(devices) if devices else 0
        
        return total_count
    except NameError:
        return 0


def format_device_info(device):
    """
    Format device information into a readable string.
    
    Args:
        device: A Device object from Nexus.
        
    Returns:
        str: Formatted device information string.
    """
    try:
        info = device.get_info()
        if not info:
            return f"Device: {device} (no info available)"
        
        # Try to get common properties
        props = []
        try:
            props.append(f"name={info.get_property_str('name')}")
        except:
            pass
        
        try:
            props.append(f"memory={info.get_property_str('memory')} bytes")
        except:
            pass
        
        props_str = ", ".join(props) if props else "no properties"
        return f"Device: {device} ({props_str})"
    except Exception as e:
        return f"Device: {device} (error getting info: {e})"


def validate_buffer_size(size):
    """
    Validate that a buffer size is valid.
    
    Args:
        size (int): The buffer size to validate.
        
    Returns:
        bool: True if size is valid, False otherwise.
        
    Raises:
        ValueError: If size is invalid.
    """
    if not isinstance(size, int):
        raise ValueError(f"Buffer size must be an integer, got {type(size)}")
    if size <= 0:
        raise ValueError(f"Buffer size must be positive, got {size}")
    if size > 2**63 - 1:  # Max size_t on 64-bit systems
        raise ValueError(f"Buffer size too large: {size}")
    return True

def get_data_type(tensor):
    """
    Get the data type of a tensor.
    
    Args:
        tensor (tensor): The tensor to get the data type of.
        
    Returns:
        int: The nexus.data_type of the tensor.

    Raises:
        ValueError: If the tensor is not a numpy or torch tensor.
    """

    if torch_loaded:
        import torch
        if isinstance(tensor, torch.Tensor):
            if tensor.dtype == torch.bfloat16:
                return nexus.data_type.BF16
            elif tensor.dtype == torch.float16:
                return nexus.data_type.F16
            elif tensor.dtype == torch.float32:
                return nexus.data_type.F32
            elif tensor.dtype == torch.float64:
                return nexus.data_type.F64
            elif tensor.dtype == torch.int32:
                return nexus.data_type.I32
            elif tensor.dtype == torch.int64:
                return nexus.data_type.I64
            elif tensor.dtype == torch.uint8:
                return nexus.data_type.U8
            elif tensor.dtype == torch.int8:
                return nexus.data_type.I8
            elif tensor.dtype == torch.uint16:
                return nexus.data_type.U16
            return nexus.data_type.Undefined
    if numpy_loaded:
        import numpy
        if isinstance(tensor, numpy.ndarray):
            if tensor.dtype == numpy.bfloat16:
                return nexus.data_type.BF16
            elif tensor.dtype == numpy.float16:
                return nexus.data_type.F16
            elif tensor.dtype == numpy.float32:
                return nexus.data_type.F32
            elif tensor.dtype == numpy.float64:
                return nexus.data_type.F64
            elif tensor.dtype == numpy.int32:
                return nexus.data_type.I32
            elif tensor.dtype == numpy.int64:
                return nexus.data_type.I64
            elif tensor.dtype == numpy.uint8:
                return nexus.data_type.U8
            elif tensor.dtype == numpy.int8:
                return nexus.data_type.I8
            elif tensor.dtype == numpy.uint16:
                return nexus.data_type.U16
            return nexus.data_type.Undefined
    raise ValueError("numpy or torch is not loaded")