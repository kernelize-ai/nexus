# Nexus Python API

The Nexus Python API provides a modern, object-oriented interface for cross-platform hardware accelerator programming. It enables device discovery, memory management, kernel execution, and property queries in a unified way across multiple backends (CUDA, HIP, Metal, CPU, etc.).

## Installation

1. **Build Nexus with Python bindings:**
   ```bash
   cd /path/to/nexus
   mkdir build && cd build
   cmake .. -DNEXUS_BUILD_PYTHON_MODULE=ON
   make
   ```

2. **Install Python dependencies:**
   ```bash
   pip install numpy
   ```

3. **Import Nexus in Python:**
   ```python
   import nexus
   ```

---

## API Overview

### Top-Level Functions

- `nexus.get_runtimes()`: Returns a list of available runtime backends (CUDA, Metal, CPU, etc.).
- `nexus.get_device_info()`: Returns the device info database.
- `nexus.lookup_device_info(name)`: Lookup device info by name.
- `nexus.create_buffer(size)` / `nexus.create_buffer(data)`: Create a system buffer from size or numpy array.

### Main Classes

#### Runtime
Represents a backend runtime (e.g., CUDA, Metal, CPU).
- `get_devices()`: List all devices for this runtime.
- `get_device(id)`: Get a device by index.

#### Device
Represents a hardware device.
- `get_info()`: Get device properties (as a Properties object).
- `create_buffer(data or size)`: Create a buffer on this device.
- `copy_buffer(buffer)`: Copy a buffer to this device.
- `create_event(event_type=nexus.event_type.Shared)`: Create an event for synchronization.
- `get_events()`: Get all events associated with this device.
- `load_library(filepath)`: Load a kernel library.
- `create_schedule()`: Create a command schedule.
- `create_stream()`: Create a command stream (if supported).

#### Buffer
Represents a memory buffer.
- `copy(host_array)`: Copy buffer contents to a numpy array or host buffer.
- `get_property_*`: Query buffer properties.

#### Library
Represents a loaded kernel library.
- `get_kernel(name)`: Get a kernel by name.

#### Kernel
Represents a compiled kernel.

#### Event
Represents a synchronization primitive.
- `signal(value=1)`: Signal the event with a specific value.
- `wait(value=1)`: Wait for the event to be signaled with a specific value.
- `get_property_*`: Query event properties.

#### Schedule
Represents a command schedule.
- `create_command(kernel)`: Create a command for a kernel.
- `create_signal_command(event, value=1)`: Create a signal command for an event.
- `create_wait_command(event, value=1)`: Create a wait command for an event.
- `run(stream=None, blocking=True)`: Execute the schedule.

#### Command
Represents a kernel execution command.
- `set_buffer(index, buffer)`: Set a buffer as a kernel argument.
- `finalize(group_size, grid_size)`: Finalize the command for execution.
- `get_event()`: Get the associated event (for signal/wait commands).

#### Properties
Represents a set of properties (device, buffer, etc.).
- `get_str(name or path)`: Get a string property.
- `get_int(name or path)`: Get an integer property.
- `get_str_vec(path)`: Get a list of string properties.

---

## Property Query Methods

Nexus provides a flexible and powerful property system for querying metadata and capabilities of all major objects (Device, Buffer, Library, Kernel, Schedule, Command, etc.). The property system supports both integer property IDs (enums) and string names, as well as hierarchical property paths.

### General Pattern

All major objects expose the following property query methods:

- **`get_property_str(name_or_enum)`**  
  Returns the property value as a string.

- **`get_property_int(name_or_enum)`**  
  Returns the property value as an integer.

- **`get_property_flt(name_or_enum)`**  
  Returns the property value as a float (if applicable).

- **`get_property_str(path: List[str] or List[int])`**  
  Returns a string property from a hierarchical path.

- **`get_property_int(path: List[str] or List[int])`**  
  Returns an integer property from a hierarchical path.

- **`get_property_flt(path: List[str] or List[int])`**  
  Returns a float property from a hierarchical path.

- **`get_property_str_vec(path: List[str] or List[int])`**  
  Returns a list of string properties from a hierarchical path.

- **`get_property_int_vec(path: List[str] or List[int])`**  
  Returns a list of integer properties from a hierarchical path.

- **`get_property_flt_vec(path: List[str] or List[int])`**  
  Returns a list of float properties from a hierarchical path.

### Usage Examples

#### Querying Device Properties

```python
info = device.get_info()

# By string name
name = info.get_str("Name")
vendor = info.get_str("Vendor")

# By enum (from nexus.property)
arch = info.get_str(nexus.property.Architecture)

# By hierarchical path
l2_size = info.get_int(["MemorySubsystem", "MemoryTypes", 1, "Size"])
```

#### Querying Buffer Properties

```python
size = buffer.get_property_int("Size")
dtype = buffer.get_property_str("DataType")
```

#### Querying Library and Kernel Properties

```python
lib_type = library.get_property_str("Type")
kernel_name = kernel.get_property_str("Name")
```

#### Querying Schedule and Command Properties

```python
sched_status = schedule.get_property_str("Status")
cmd_args = command.get_property_int_vec("Arguments")
```

### Notes

- If a property is not found, a `RuntimeError` is raised.
- Property names are case-sensitive and must match the schema or backend.
- Enum values are available in the `nexus.property` submodule for convenience and type safety.
- Hierarchical paths allow access to nested properties, such as memory subsystem details or device features.

---

## Event Types and Synchronization

Nexus supports three types of events for different synchronization patterns:

### Event Types

- **`nexus.event_type.Shared`**: Shared events for cross-queue synchronization
- **`nexus.event_type.Signal`**: Signal events for simple completion notifications  
- **`nexus.event_type.Fence`**: Fence events for kernel completion synchronization

### Basic Event Usage

```python
import nexus

# Create an event
event = device.create_event(nexus.event_type.Shared)

# Signal the event
event.signal(1)

# Wait for the event
event.wait(1)
```

### Event-Based Synchronization

```python
# Create event and schedule
event = device.create_event()
schedule = device.create_schedule()

# Add kernel command
cmd = schedule.create_command(kernel)
cmd.set_buffer(0, input_buffer)
cmd.finalize(256, 1024)

# Add signal command
signal_cmd = schedule.create_signal_command(event, 1)

# Run schedule non-blocking
schedule.run(blocking=False)

# Wait for completion
event.wait(1)
```

For detailed event documentation, see [Event API](Event_API.md).

### Advanced: Property Vectors

Some properties are arrays or lists (e.g., supported memory types, available kernels). Use the `_vec` methods to retrieve these as Python lists.

```python
mem_types = info.get_str_vec(["MemorySubsystem", "SupportedMemoryTypes"])
```

---

## Summary Table

| Method                        | Return Type         | Description                                 |
|-------------------------------|--------------------|---------------------------------------------|
| `get_property_str(key)`       | `str`              | Get property as string                      |
| `get_property_int(key)`       | `int`              | Get property as integer                     |
| `get_property_flt(key)`       | `float`            | Get property as float                       |
| `get_property_str_vec(key)`   | `List[str]`        | Get property as list of strings             |
| `get_property_int_vec(key)`   | `List[int]`        | Get property as list of integers            |
| `get_property_flt_vec(key)`   | `List[float]`      | Get property as list of floats              |

- `key` can be a string, enum, or a list (for hierarchical paths).

---

## Example Usage

```python
import nexus
import numpy as np

# Get available runtimes and devices
runtimes = nexus.get_runtimes()
runtime = runtimes[0]
devices = runtime.get_devices()
device = devices[0]

# Create a buffer from numpy array
data = np.ones(1024, dtype=np.float32)
buf = device.create_buffer(data)

# Load a kernel library and get a kernel
lib = device.load_library("kernel.so")
kernel = lib.get_kernel("add_vectors")

# Create a schedule and command
schedule = device.create_schedule()
command = schedule.create_command(kernel)
command.set_buffer(0, buf)
command.finalize(32, 1024)

# Run the schedule
schedule.run()
```

---

## Status and Error Codes

Nexus exposes status codes as enums in `nexus.status` (e.g., `nexus.status.NXS_Success`) and properties as enums in `nexus.property`.

---

## Design Notes

- All objects are reference-counted and managed by the underlying C++ backend.
- Handles numpy arrays and Python buffers for efficient data transfer.
- Supports device and runtime enumeration, property queries, and kernel execution in a backend-agnostic way.

---

## Advanced Features

- **Streams:** For asynchronous execution (if supported by backend).
- **Multiple Devices:** Iterate over all runtimes and devices for multi-accelerator systems.
- **Device Info Database:** Use `nexus.get_device_info()` and `nexus.lookup_device_info(name)` for architecture-aware programming.

---

## See Also

- [Unit Tests](../test/python/README.md)

---

**Nexus Python API enables portable, high-performance accelerator programming with a modern, Pythonic interface.** 