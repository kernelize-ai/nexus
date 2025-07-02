# GitHub Actions Workflows

This directory contains GitHub Actions workflows for building and testing the nexus project.

## Workflows

### CI (`ci.yml`)
A comprehensive CI workflow that builds the project on all major platforms:

- **Linux** (Ubuntu): GCC compiler, Debug and Release builds
- **macOS**: Clang compiler, Debug and Release builds  
- **Windows**: MSVC compiler, Debug and Release builds

**Features:**
- Matrix builds for multiple configurations
- Automatic dependency installation
- CMake configuration with Python bindings and plugins enabled
- Parallel builds for faster execution
- Test execution with CTest

### Build (`build.yml`)
An extended build workflow with additional configurations:

- **Linux**: GCC and Clang compilers, Debug and Release builds
- **macOS**: Clang compiler, Debug and Release builds
- **Windows**: MSVC compiler, Debug and Release builds
- **Linux with CUDA**: CUDA 11.8 and 12.0 support
- **Linux with ROCm**: HIP support for AMD GPUs

**Features:**
- GPU compute support (CUDA, ROCm/HIP)
- Multiple compiler testing
- Comprehensive platform coverage

## Build Configuration

Both workflows configure CMake with the following options:
- `NEXUS_BUILD_PYTHON_MODULE=ON`: Builds Python bindings
- `NEXUS_BUILD_PLUGINS=ON`: Builds runtime plugins (Metal on macOS, CUDA/HIP on Linux)
- `CMAKE_BUILD_TYPE`: Set to Debug or Release based on matrix configuration

## Dependencies

The workflows automatically install:
- **Build tools**: CMake, compilers (GCC, Clang, MSVC)
- **Python**: Python 3.x with development headers
- **Python packages**: pybind11 for Python bindings
- **Platform-specific**: libc++ on Linux, Metal framework on macOS

## Running Locally

To run the same build process locally:

### Linux
```bash
sudo apt-get install build-essential cmake python3 python3-dev python3-pip libc++-dev libc++abi-dev
python3 -m pip install pybind11
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DNEXUS_BUILD_PYTHON_MODULE=ON -DNEXUS_BUILD_PLUGINS=ON
make -j$(nproc)
ctest --output-on-failure
```

### macOS
```bash
brew install cmake python@3.11 pybind11
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DNEXUS_BUILD_PYTHON_MODULE=ON -DNEXUS_BUILD_PLUGINS=ON
make -j$(sysctl -n hw.ncpu)
ctest --output-on-failure
```

### Windows
```bash
# Install Visual Studio 2022 with C++ workload
python -m pip install pybind11
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -DNEXUS_BUILD_PYTHON_MODULE=ON -DNEXUS_BUILD_PLUGINS=ON
cmake --build . --config Release --parallel
ctest --output-on-failure -C Release
``` 