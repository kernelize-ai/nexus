"""isort:skip_file"""
__version__ = '0.0.1'

# ---------------------------------------
# Note: import order is significant here.

import os
_NEXUS_PACKAGE_DIR = os.path.dirname(os.path.abspath(__file__))

os.environ['NEXUS_HOME'] = _NEXUS_PACKAGE_DIR
os.environ['NEXUS_RUNTIME_PATH'] = os.path.join(_NEXUS_PACKAGE_DIR, 'runtime_libs')
os.environ['NEXUS_DEVICE_PATH'] = os.path.join(_NEXUS_PACKAGE_DIR, 'device_lib')

from ._C.libnexus import *

# Import utility functions
from . import utils
from .utils import (
    version_info,
    list_available_runtimes,
    get_device_count,
    format_device_info,
    validate_buffer_size,
    get_data_type,
)

__all__ = [
    'version_info',
    'list_available_runtimes',
    'get_device_count',
    'format_device_info',
    'validate_buffer_size',
    'get_data_type',
]


