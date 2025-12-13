import nexus
import sys

"""
Utility functions for the Nexus module.

This module provides helper functions and utilities for working with Nexus.
"""

numpy_loaded = 'numpy' in sys.modules
torch_loaded = 'torch' in sys.modules

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


def format_device_info(device):
    """
    TODO: Format device information into a readable string.
    
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
        
        props_str = ", ".join(props) if props else "no properties"
        return f"Device: {device} ({props_str})"
    except Exception as e:
        return f"Device: {device} (error getting info: {e})"


def get_data_type(obj):
    """
    Get the data type of a tensor.
    
    Args:
        tensor (tensor): The tensor to get the data type of.
        
    Returns:
        int: The nexus.data_type of the tensor.

    Raises:
        ValueError: If the tensor is not a numpy or torch tensor.
    """
    if isinstance(obj, nexus.buffer):
        return obj.dtype
    if isinstance(obj, nexus.data_type.nxs_data_type):
        return obj
    if torch_loaded:
        import torch
        tdtype = None
        if isinstance(obj, torch.Tensor):
            tdtype = obj.dtype
        elif isinstance(obj, torch.dtype):
            tdtype = obj
        if tdtype == torch.bfloat16:
            return nexus.data_type.bfloat16
        elif tdtype == torch.float16:
            return nexus.data_type.float16
        elif tdtype == torch.float32:
            return nexus.data_type.float32
        elif tdtype == torch.float64:
            return nexus.data_type.float64
        elif tdtype == torch.int32:
            return nexus.data_type.int32
        elif tdtype == torch.int64:
            return nexus.data_type.int64
        elif tdtype == torch.uint8:
            return nexus.data_type.uint8
        elif tdtype == torch.int8:
            return nexus.data_type.int8
        elif tdtype == torch.uint16:
            return nexus.data_type.uint16
        return nexus.data_type.undefined
    if numpy_loaded:
        import numpy
        tdtype = None
        if isinstance(obj, numpy.ndarray):
            tdtype = obj.dtype
        elif isinstance(obj, numpy.dtype):
            tdtype = obj
        if tdtype == numpy.bfloat16:
            return nexus.data_type.bfloat16
        elif tdtype == numpy.float16:
            return nexus.data_type.float16
        elif tdtype == numpy.float32:
            return nexus.data_type.float32
        elif tdtype == numpy.float64:
            return nexus.data_type.float64
        elif tdtype == numpy.int32:
            return nexus.data_type.int32
        elif tdtype == numpy.int64:
            return nexus.data_type.int64
        elif tdtype == numpy.uint8:
            return nexus.data_type.uint8
        elif tdtype == numpy.int8:
            return nexus.data_type.int8
        elif tdtype == numpy.uint16:
            return nexus.data_type.uint16
        return nexus.data_type.undefined
    raise ValueError("numpy or torch is not loaded")

def get_data_type_size(obj):
    dtype = get_data_type(obj)
    if dtype == nexus.data_type.float32:
        return 4
    elif dtype == nexus.data_type.int32:
        return 4
    elif dtype == nexus.data_type.float16:
        return 2
    elif dtype == nexus.data_type.bfloat16:
        return 2
    elif dtype == nexus.data_type.float8:
        return 1
    elif dtype == nexus.data_type.bfloat8:
        return 1
    elif dtype == nexus.data_type.uint32:
        return 4
    elif dtype == nexus.data_type.int16:
        return 2
    elif dtype == nexus.data_type.uint16:
        return 2
    elif dtype == nexus.data_type.int8:
        return 1
    elif dtype == nexus.data_type.uint8:
        return 1
    elif dtype == nexus.data_type.int4:
        return 1
    elif dtype == nexus.data_type.uint4:
        return 1
    elif dtype == nexus.data_type.float64:
        return 8
    elif dtype == nexus.data_type.int64:
        return 8
    elif dtype == nexus.data_type.uint64:
        return 8
    else:
        raise ValueError("Invalid data type")
