# Build and CI Documentation

This document covers the build process, continuous integration setup, and development workflow for the Nexus framework.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Building from Source](#building-from-source)
- [Continuous Integration](#continuous-integration)
- [Development Workflow](#development-workflow)
- [Troubleshooting](#troubleshooting)

## Prerequisites

### System Requirements

- **CMake**: Version 3.18 or higher
- **C++ Compiler**: 
  - Linux: GCC 9+ or Clang 10+
  - macOS: Clang (Xcode Command Line Tools)
  - Windows: Visual Studio 2019+ with C++ workload
- **Python**: 3.8+ with development headers
- **Build Tools**: Make, Ninja (optional)

### Dependencies

#### Linux
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake python3 python3-dev python3-pip python3-pybind11
sudo apt-get install -y libc++-dev libc++abi-dev
```

#### macOS
```bash
brew update
brew install cmake python@3.11 pybind11
```

#### Windows
```bash
# Install Visual Studio 2022 with C++ workload
python -m pip install pybind11
```

## Building from Source

### Basic Build

1. **Clone the repository**:
   ```bash
   git clone --recursive https://github.com/your-org/nexus.git
   cd nexus
   ```

2. **Create build directory**:
   ```bash
   mkdir build
   cd build
   ```

3. **Configure with CMake**:
   ```bash
   cmake .. -DCMAKE_BUILD_TYPE=Release -DNEXUS_BUILD_PYTHON_MODULE=ON -DNEXUS_BUILD_PLUGINS=ON
   ```

4. **Build the project**:
   ```bash
   make -j$(nproc)  # Linux
   make -j$(sysctl -n hw.ncpu)  # macOS
   cmake --build . --config Release --parallel  # Windows
   ```

5. **Run tests**:
   ```bash
   ctest --output-on-failure
   ```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | Debug | Build type (Debug, Release, RelWithDebInfo) |
| `NEXUS_BUILD_PYTHON_MODULE` | ON | Build Python bindings |
| `NEXUS_BUILD_PLUGINS` | ON | Build runtime plugins |
| `NEXUS_ENABLE_LOGGING` | ON | Enable Nexus logging |

### Platform-Specific Builds

#### Linux with CUDA
```bash
# Install CUDA toolkit first
cmake .. -DCMAKE_BUILD_TYPE=Release -DNEXUS_BUILD_PYTHON_MODULE=ON -DNEXUS_BUILD_PLUGINS=ON
```

#### Linux with ROCm/HIP
```bash
# Install ROCm first
cmake .. -DCMAKE_BUILD_TYPE=Release -DNEXUS_BUILD_PYTHON_MODULE=ON -DNEXUS_BUILD_PLUGINS=ON
```

#### macOS with Metal
```bash
# Metal support is automatically enabled on macOS
cmake .. -DCMAKE_BUILD_TYPE=Release -DNEXUS_BUILD_PYTHON_MODULE=ON -DNEXUS_BUILD_PLUGINS=ON
```

## Continuous Integration

### GitHub Actions

The project uses GitHub Actions for continuous integration. Workflows are located in `.github/workflows/`.

#### CI Workflow (`ci.yml`)

**Triggers**:
- Push to `main` branch
- Pull requests to `main` branch (opened, synchronize, reopened)

**Platforms**:
- **Linux** (Ubuntu): GCC compiler, Debug and Release builds
- **macOS**: Clang compiler, Debug and Release builds

**Features**:
- Matrix builds for multiple configurations
- Automatic dependency installation
- CMake configuration with Python bindings and plugins enabled
- Parallel builds for faster execution
- Test execution with CTest

#### Workflow Steps

1. **Checkout**: Repository with submodules
2. **Install Dependencies**: Platform-specific package installation
3. **Configure CMake**: Set up build configuration
4. **Build**: Compile with parallel execution
5. **Test**: Run CTest suite

### Local CI Simulation

To simulate the CI environment locally:

```bash
# Linux
docker run -it --rm -v $(pwd):/workspace ubuntu:latest bash
cd /workspace
sudo apt-get update
sudo apt-get install -y build-essential cmake python3 python3-dev python3-pip python3-pybind11 libc++-dev libc++abi-dev
python3 -m pip install -r requirements.txt
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DNEXUS_BUILD_PYTHON_MODULE=ON -DNEXUS_BUILD_PLUGINS=ON
make -j$(nproc)
ctest --output-on-failure
```

## Development Workflow

### Setting Up Development Environment

1. **Fork and clone** the repository
2. **Install dependencies** for your platform
3. **Create a development branch**:
   ```bash
   git checkout -b feature/your-feature-name
   ```

### Making Changes

1. **Make your changes** in the codebase
2. **Build and test locally**:
   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Debug -DNEXUS_BUILD_PYTHON_MODULE=ON -DNEXUS_BUILD_PLUGINS=ON
   make -j$(nproc)
   ctest --output-on-failure
   ```

3. **Commit your changes**:
   ```bash
   git add .
   git commit -m "Add feature: description of changes"
   ```

4. **Push and create pull request**:
   ```bash
   git push origin feature/your-feature-name
   ```

### Code Quality

- **Formatting**: Use clang-format for C++ code
- **Linting**: Enable compiler warnings and treat as errors
- **Testing**: Add tests for new features
- **Documentation**: Update relevant documentation

## Troubleshooting

### Common Build Issues

#### CMake Configuration Errors

**Problem**: CMake can't find Python or pybind11
```bash
# Solution: Install Python development headers
sudo apt-get install python3-dev  # Linux
brew install python@3.11  # macOS
```

**Problem**: Compiler not found
```bash
# Solution: Install build tools
sudo apt-get install build-essential  # Linux
xcode-select --install  # macOS
```

#### Compilation Errors

**Problem**: Missing C++17 support
```bash
# Solution: Use a newer compiler
export CC=gcc-9
export CXX=g++-9
```

**Problem**: pybind11 compilation errors
```bash
# Solution: Update pybind11
python3 -m pip install --upgrade pybind11
```

#### Test Failures

**Problem**: Tests fail on specific platforms
```bash
# Solution: Check platform-specific requirements
# Some tests may require specific hardware (GPU, etc.)
```

### Platform-Specific Issues

#### Linux
- Ensure libc++ is installed for Clang builds
- Check CUDA/ROCm installation for GPU support

#### macOS
- Install Xcode Command Line Tools
- Metal framework is automatically available

#### Windows
- Use Visual Studio 2019+ with C++ workload
- Ensure Python is in PATH

### Getting Help

1. **Check existing issues** on GitHub
2. **Review CI logs** for build errors
3. **Test locally** with the same configuration as CI
4. **Create a minimal reproduction** of the issue

## Related Documentation

- [Core API](Core_API.md) - C++ API documentation
- [Python API](Python_API.md) - Python bindings documentation
- [Plugin API](Plugin_API.md) - Plugin development guide
- [JSON API](JSON_API.md) - JSON interface documentation 