# Nexus Device API

The Nexus Device API provides a clean, standardized Interface to Device Discovery, Characterization and Kernel Deployment.

## Interfaces

There are 4 interfaces in Nexus, 2 User APIs and 2 Vendor APIs.

User APIs:
* Python API
* C++ Source API

Vendor APIs:
* JSON DB
* Runtime Plugin C-API

### Python API

The Python API is designed to be intuitive with full device discovery, characterization and kernel execution.

```python
import nexus

runtimes = nexus.get_runtimes()
rt0 = runtimes.get(0)
rt0_name = rt0.get_property_str('Name')

dev0 = rt0.get_device(0)
dev0_arch = dev0.get_property_str('Architecture')

buf0 = dev0.create_buffer(tensor0)
buf1 = dev0.create_buffer((1024,1024), dtype='fp16')

# Create event for synchronization
event = dev0.create_event(nexus.event_type.Shared)

sched0 = dev0.create_schedule()

cmd0 = sched0.create_command(kernel)
signal_cmd = sched0.create_signal_command(event, 1)

sched0.run(blocking=False)
event.wait(1)  # Wait for completion
```

### C++ Source API

The C++ Source API provides direct access to all API objects with clean interface and garbage collection.

```
// insert test/cpp/main.cpp
```

### JSON DB

The JSON DB interface provides deep device/system characteristics to improve compilation and runtime distribution. There should be a device_lib.json for each architecture. 
The file name follows the convention `<vendor_name>-<device_type>-<architecture>.json`. This should correlate with querying the device:

```c++
auto vendor = device.getProperty<std::string>("Vendor");
auto type = device.getProperty<std::string>("Type");
auto arch = device.getProperty<std::string>("Architecture");
```

// see schema/gpu_architecture_schema.json


### Runtime Plugin C-API

The Runtime Plugin C-API is a thin wrapper for clean dynamic library loading to call into vendor specific runtimes.

// See plugins/metal/metal_runtime.cpp for example


## Building Nexus

### Quick Start

For a quick setup, see the [Quick Start Guide](docs/Quick_Start.md).

### Detailed Build Instructions

First clone the repo with submodules:

```shell
git clone --recursive https://github.com/kernelize-ai/nexus.git
cd nexus
```

Then build with CMake:

```shell
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DNEXUS_BUILD_PYTHON_MODULE=ON -DNEXUS_BUILD_PLUGINS=ON
make -j$(nproc)  # Linux/macOS
# or
cmake --build . --config Release --parallel  # Windows
```

For detailed build instructions, dependencies, and troubleshooting, see the [Build and CI Documentation](docs/Build_and_CI.md).

### Building the python packages

For building the development package in a virtual environment:

```shell
python3 -m venv .venv --prompt nexus
source .venv/bin/activate  # Linux/macOS
# or
.venv\Scripts\activate  # Windows
pip install -r requirements.txt

pip install -e .
```

For building and installing the release package in a virtual environment:

```shell
python -m build

python3 -m venv .venv --prompt nexus
source .venv/bin/activate  # Linux/macOS
# or
.venv\Scripts\activate  # Windows
pip install -r requirements.txt

pip install dist/nexus-*.whl
```

## Testing

Run the test suite:

```shell
cd build
ctest --output-on-failure
```

For Python-specific tests:

```shell
python test/pynexus/test.py
```

## Continuous Integration

The project uses GitHub Actions for continuous integration, building on:
- **Linux** (Ubuntu): GCC compiler, Debug and Release builds
- **macOS**: Clang compiler, Debug and Release builds

See the [Build and CI Documentation](docs/Build_and_CI.md) for details on the CI setup and how to run builds locally.

## Documentation

- [Quick Start Guide](docs/Quick_Start.md) - Get up and running quickly
- [Core API](docs/Core_API.md) - C++ API documentation
- [Python API](docs/Python_API.md) - Python bindings documentation
- [Plugin API](docs/Plugin_API.md) - Plugin development guide
- [JSON API](docs/JSON_API.md) - JSON interface documentation
- [Build and CI](docs/Build_and_CI.md) - Build instructions and CI setup
