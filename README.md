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

sched0 = dev0.create_schedule()

cmd0 = sched0.create_command(kernel)
...

sched0.run()
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

First clone the repo.

```shell
git clone https://github.com/kernelize-ai/nexus.git
cd nexus
## ??
git submodule update --init
```

Then create the environment.

```shell
python3 -m venv .venv --prompt nexus
source .venv/bin/activate
```

Now build and install in your python.

```shell
pip install -e .
```

## Testing

Try test/pynexus/test.py
(still needs kernel.so)
