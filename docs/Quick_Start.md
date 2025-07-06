# Quick Start Guide

This guide will help you get the Nexus framework up and running quickly.

## Prerequisites

- **CMake** 3.18+
- **C++ Compiler** (GCC 9+, Clang 10+, or MSVC 2019+)
- **Python** 3.8+ with development headers
- **Git** with submodule support

## Quick Setup

### 1. Clone the Repository

```bash
git clone --recursive https://github.com/your-org/nexus.git
cd nexus
```

### 2. Install Dependencies

#### Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake python3 python3-dev python3-pip python3-pybind11 libc++-dev libc++abi-dev
python3 -m pip install -r requirements.txt
```

#### macOS
```bash
brew install cmake python@3.11 pybind11
python3 -m pip install -r requirements.txt
```

#### Windows
```bash
# Install Visual Studio 2022 with C++ workload
python -m pip install -r requirements.txt
```

### 3. Build the Project

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DNEXUS_BUILD_PYTHON_MODULE=ON -DNEXUS_BUILD_PLUGINS=ON
make -j$(nproc)  # Linux/macOS
# or
cmake --build . --config Release --parallel  # Windows
```

### 4. Run Tests

```bash
ctest --output-on-failure
```

## What's Built

The build process creates:

- **Core Library**: `libnexus-api.so` (Linux), `libnexus-api.dylib` (macOS), `nexus-api.dll` (Windows)
- **Python Module**: `nexus.so` (Linux/macOS), `nexus.pyd` (Windows)
- **Runtime Plugins**: CPU, Metal (macOS), CUDA/HIP (Linux)
- **Tests**: C++ and Python test suites

## Next Steps

- Read the [Core API](Core_API.md) documentation for C++ development
- Check out the [Python API](Python_API.md) for Python bindings
- Explore [Plugin API](Plugin_API.md) for custom hardware backends
- See [Build and CI](Build_and_CI.md) for detailed build instructions

## Troubleshooting

### Common Issues

**CMake can't find Python**
```bash
# Install Python development headers
sudo apt-get install python3-dev  # Linux
brew install python@3.11  # macOS
```

**Build fails with C++17 errors**
```bash
# Use a newer compiler
export CC=gcc-9
export CXX=g++-9
```

**Tests fail**
```bash
# Check if you have required hardware (GPU for CUDA/Metal tests)
# Some tests may be platform-specific
```

For more detailed troubleshooting, see the [Build and CI](Build_and_CI.md) documentation. 