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
