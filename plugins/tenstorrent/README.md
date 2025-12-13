# Tenstorrent Plugin for Nexus

The Tenstorrent plugin provides a Nexus runtime backend for Tenstorrent AI accelerators using the TT Metalium API. This plugin enables the Nexus framework to discover, manage, and execute kernels on Tenstorrent devices through a unified interface.

## Overview

This plugin implements the Nexus Plugin API (C API) to provide hardware abstraction for Tenstorrent NPUs. It maps Nexus concepts (devices, buffers, libraries, kernels, schedules, commands) to TT Metalium distributed mesh operations, enabling seamless integration of Tenstorrent accelerators into the Nexus ecosystem.

## Architecture

### Nexus to TT Metalium Mapping

| Nexus Concept | TT Metalium Implementation |
|---------------|----------------------------|
| Runtime | Singleton `TTRuntime` managing all devices |
| Device | `ttmd::MeshDevice` (unit mesh device) |
| Buffer | `ttmd::MeshBuffer` (distributed/replicated DRAM buffer) |
| Library | `TTLibrary` (JIT kernel compilation from source files) |
| Kernel | `TTKernel` (wraps reader/writer/compute kernels) |
| Stream | `ttmd::MeshCommandQueue` (command queue) |
| Schedule | `TTSchedule` (mesh workload collection) |
| Command | `TTCommand` (program with core range assignment) |

### Key Components

- **TTRuntime**: Manages device discovery, object pools, and resource allocation
- **TTDevice**: Wraps TT Metalium mesh devices with lazy initialization
- **TTBuffer**: Handles device memory allocation with tile-based padding
- **TTLibrary**: JIT compiles kernels from source files (reader/writer/compute)
- **TTCommand**: Builds TT Metalium programs with core range placement
- **TTSchedule**: Collects and executes commands as mesh workloads

## Dependencies

### Required

- **TT Metalium**: Tenstorrent's C++ API for device programming
  - Set `TT_METAL_CMAKE_PATH` environment variable to TT Metalium installation path
  - The plugin expects TT Metalium to be installed with CMake support

### Build System

- CMake 3.x or later
- C++ compiler with C++17 support
- Nexus API headers (`nexus-api.h`)

## Building

### Prerequisites

1. Install TT Metalium and set the environment variable:
   ```bash
   export TT_METAL_CMAKE_PATH=/path/to/tt-metalium/installation
   ```

2. Ensure Nexus API headers are available in the build system.

### Build Instructions

The plugin is built as part of the Nexus build system. From the Nexus root directory:

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

The plugin will be built as `libtt_plugin.so` (Linux) in the `runtime_libs` directory if TT Metalium is found.

### CMake Configuration

The `CMakeLists.txt` automatically:
- Searches for TT Metalium using the `TT_METAL_CMAKE_PATH` environment variable
- Links against `TT::Metalium` target
- Configures linker options to exclude library symbols (Linux)

If TT Metalium is not found, the plugin will not be built (warning message will be displayed).

## Usage

### Device Discovery

```python
import nexus

# Get all available runtimes
runtimes = nexus.get_runtimes()

# Find Tenstorrent runtime (name="tt-metal")
tt_runtime = None
for rt in runtimes:
    if rt.get_property_str('Name') == 'tt-metal':
        tt_runtime = rt
        break

# Get device count
num_devices = tt_runtime.get_property_int('Size')

# Get first device
device = tt_runtime.get_device(0)
arch = device.get_property_str('Architecture')  # e.g., "Wormhole", "Grayskull"
num_cores = device.get_property_int('Size')  # Grid size (x * y)
```

### Buffer Management

```python
# Create buffer with data
import numpy as np
data = np.array([1.0, 2.0, 3.0], dtype=np.float32)
buffer = device.create_buffer(data) # auto-detects datatype

# Create empty buffer
buffer = device.create_buffer((1024, 1024), dtype=nexus.datatype.F16)

# Copy buffer to host
host_data = buffer.copy_to_host()
```

### Kernel Execution

```python
# Load library from source file
library = device.create_library('my_kernel.cpp')

# Get kernel
kernel = library.get_kernel('my_kernel')

# Create schedule
schedule = device.create_schedule()

# Create command
command = schedule.create_command(kernel)

# Set buffer arguments
command.set_arg(0, input_buffer)
command.set_arg(1, output_buffer)

# Set scalar arguments
command.set_arg(2, value=42, name='param')

# Finalize command with grid/group sizes
command.finalize(
    grid_size=(8, 8, 1),    # Grid dimensions
    group_size=(1, 1, 1)     # Group dimensions (not used by TT, must be single-threaded)
)

# Run schedule
schedule.run(blocking=True)

# Get execution time
elapsed_time = schedule.get_property_float('ElapsedTime')
```

## Data Format Support

The plugin supports the following Nexus data types, mapped to TT Metalium formats:

| Nexus Type | TT Metalium Format |
|------------|-------------------|
| `F32` | `Float32` |
| `F16` | `Float16` |
| `BF16` | `Float16_b` |
| `F8` | `Bfp8` |
| `BF8` | `Bfp8_b` |
| `I32` | `Int32` |
| `U32` | `UInt32` |
| `I16` | `Int16` |
| `U16` | `UInt16` |
| `I8` | `Int8` |
| `U8` | `UInt8` |

## Kernel Development

### Kernel Source Structure

TT Metalium kernels use a three-kernel model:
- **Reader Kernel**: Loads data from DRAM to L1 (RISCV_0 processor)
- **Compute Kernel**: Performs computation (Compute processor)
- **Writer Kernel**: Writes results from L1 to DRAM (RISCV_1 processor)

Your kernel source file should use preprocessor defines to differentiate:

```cpp
#ifdef READER_KERNEL
// Reader kernel code
void main() {
    // Load data from DRAM to L1 circular buffers
}
#endif

#ifdef COMPUTE_KERNEL
// Compute kernel code
void main() {
    // Perform computation using L1 buffers
}
#endif

#ifdef WRITER_KERNEL
// Writer kernel code
void main() {
    // Write results from L1 to DRAM
}
#endif
```

### Circular Buffer Configuration

Commands can configure circular buffers using constants:

```python
# Set circular buffer constant
command.set_const(0, value=tile_count, name='CB', dtype='i32')
# This creates a circular buffer with:
# - Index: 0
# - Tile count: tile_count
# - Tile size: 1024 * data_type_size
# - Data format: from dtype
```

### Runtime Arguments

Kernel runtime arguments are passed as 32-bit values. The plugin automatically adds grid coordinates:
- `args[N]`: Grid X coordinate
- `args[N+1]`: Grid Y coordinate  
- `args[N+2]`: Grid Z coordinate

## Core Range Placement

The plugin automatically places commands across device cores:

- Commands are placed sequentially in the device's core grid
- Each command's grid size determines how many core rows it occupies
- The `placeCommand()` function maps command grid sizes to core ranges
- Commands that exceed available cores will fail (future: enqueue to new workload)

## Features

- **Multi-Device Support**: Automatically discovers all available Tenstorrent devices
- **Distributed Buffers**: Supports replicated and distributed buffer configurations
- **JIT Compilation**: Compiles kernels from source files at runtime
- **Tile-Based Memory**: Automatic padding to tile boundaries (1024 elements)
- **Mesh Workloads**: Batches commands into efficient mesh workloads
- **Timing Support**: Measures execution time when enabled
- **Data Format Conversion**: Automatic mapping between Nexus and TT Metalium formats

## Limitations

1. **Memory Model**: 
   - Buffers are padded to tile boundaries (1024 elements)
   - Explicit host-device transfers (no unified memory)

2. **Kernel Compilation**:
   - Only file-based library loading supported
   - In-memory library loading (`nxsCreateLibrary`) not implemented
   - Requires source files with proper preprocessor defines

3. **Command Placement**:
   - Simple sequential placement algorithm
   - No gap-filling optimization for single-row commands
   - Commands exceeding device capacity fail (no automatic workload splitting)

4. **Synchronization**:
   - Streams are placeholders (no actual stream management)
   - Blocking execution only (schedules always finish before returning)

5. **Error Handling**:
   - Basic error checking with logging
   - Limited error recovery mechanisms

6. **Argument Limits**:
   - Maximum 32 arguments per kernel (enforced)

## API Implementation Status

| Function | Status | Notes |
|----------|--------|-------|
| `nxsGetRuntimeProperty` | ✅ | Returns "tt-metal" runtime info |
| `nxsGetDeviceProperty` | ✅ | Returns device architecture, grid size |
| `nxsCreateBuffer` | ✅ | Creates distributed/replicated buffers |
| `nxsCopyBuffer` | ✅ | Copies buffer to host |
| `nxsReleaseBuffer` | ✅ | Releases buffer resources |
| `nxsCreateLibrary` | ❌ | Not implemented |
| `nxsCreateLibraryFromFile` | ✅ | Loads library from source file |
| `nxsGetKernel` | ✅ | Returns kernel handle |
| `nxsCreateSchedule` | ✅ | Creates schedule for command batching |
| `nxsCreateCommand` | ✅ | Creates command with kernel |
| `nxsSetCommandArgument` | ✅ | Sets buffer arguments |
| `nxsSetCommandScalar` | ✅ | Sets scalar/constant arguments |
| `nxsFinalizeCommand` | ✅ | Finalizes command with grid/group sizes |
| `nxsRunSchedule` | ✅ | Executes schedule as mesh workload |
| `nxsCreateStream` | ⚠️ | Placeholder (returns success) |
| `nxsReleaseStream` | ⚠️ | Placeholder (returns success) |

## Logging

The plugin uses Nexus logging infrastructure with module name `"tt_runtime"`. Enable logging to see:
- Device discovery and initialization
- Buffer allocation and transfers
- Command placement and execution
- Kernel JIT compilation
- Error messages

## Future Improvements

- [ ] Implement in-memory library loading
- [ ] Add gap-filling optimization for command placement
- [ ] Implement automatic workload splitting for large commands
- [ ] Add stream-based asynchronous execution
- [ ] Support event-based synchronization
- [ ] Improve error recovery and reporting
- [ ] Add memory pooling for buffer reuse
- [ ] Support additional TT Metalium features (L1 buffers, etc.)

## References

- **Nexus Plugin API**: See `docs/Plugin_API.md` for API documentation
- **TT Metalium**: Tenstorrent's C++ API documentation
- **Example Plugins**: See `plugins/metal/` and `plugins/hip/` for reference implementations

## License

This plugin is part of the Nexus project and follows the same license terms.

