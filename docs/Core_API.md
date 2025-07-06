# Nexus Core API Documentation

## Overview

Nexus is a cross-platform GPU computing framework that provides a unified API for heterogeneous computing across multiple GPU backends including CUDA, HIP, Metal, and CPU. The framework is designed to be similar to OpenCL but with modern C++ interfaces and Python bindings.

## Table of Contents

1. [Architecture](#architecture)
2. [Core Concepts](#core-concepts)
3. [C++ API Reference](#c-api-reference)
7. [Build System](#build-system)

## Architecture

Nexus follows a hierarchical object model:

```
System
├── Runtimes
│   └── Devices
│       ├── Libraries
│       │   └── Kernels
│       └── Schedules
│           └── Commands
└── Buffers
```

## Core Concepts

### Objects and Ownership

All Nexus objects follow a shared ownership model using `std::shared_ptr` internally. Objects have hierarchical relationships where child objects are owned by parent objects.

### Properties System

Nexus uses a flexible property system for querying device capabilities, runtime information, and object metadata. Properties can be accessed by:
- Integer property IDs (similar to OpenCL)
- String names
- Hierarchical paths

### Memory Management

Buffers are the primary memory abstraction, supporting:
- Host memory allocation
- Device memory allocation
- Memory copying between host and device
- Buffer-to-buffer copying

## C++ API Reference

### Main Header

```cpp
#include <nexus.h>
```

### Core Classes

#### System

The top-level system object that manages all resources.

```cpp
namespace nexus {
    class System : Object<detail::SystemImpl> {
    public:
        System(int);
        using Object::Object;

        nxs_int getId() const override { return 0; }
        std::optional<Property> getProperty(nxs_int prop) const override;

        Runtimes getRuntimes() const;
        Buffers getBuffers() const;

        Runtime getRuntime(int idx) const;
        Buffer createBuffer(size_t sz, const void *hostData = nullptr);
        Buffer copyBuffer(Buffer buf, Device dev);
    };

    extern System getSystem();
}
```

**Methods:**
- `getRuntimes()`: Get all available runtimes
- `getRuntime(int idx)`: Get runtime by index
- `createBuffer(size_t sz, const void *hostData)`: Create a new buffer
- `copyBuffer(Buffer buf, Device dev)`: Copy buffer to device

#### Runtime

Represents a GPU runtime (CUDA, HIP, Metal, etc.).

```cpp
namespace nexus {
    class Runtime : public Object<detail::RuntimeImpl, detail::SystemImpl> {
    public:
        Runtime(detail::Impl owner, const std::string& libraryPath);
        using Object::Object;

        nxs_int getId() const override;
        Devices getDevices() const;
        Device getDevice(nxs_uint deviceId) const;
        std::optional<Property> getProperty(nxs_int prop) const override;
    };

    typedef Objects<Runtime> Runtimes;
}
```

**Methods:**
- `getDevices()`: Get all devices in this runtime
- `getDevice(nxs_uint deviceId)`: Get specific device by ID
- `getProperty(nxs_int prop)`: Get runtime properties

#### Device

Represents a physical or virtual compute device.

```cpp
namespace nexus {
    class Device : public Object<detail::DeviceImpl, detail::RuntimeImpl> {
    public:
        Device(detail::Impl base);
        using Object::Object;

        nxs_int getId() const override;
        std::optional<Property> getProperty(nxs_int prop) const override;
        Properties getInfo() const;

        // Runtime functions
        Librarys getLibraries() const;
        Schedules getSchedules() const;
        Streams getStreams() const;
        Events getEvents() const;

        Schedule createSchedule();
        Stream createStream();
        Event createEvent(nxs_event_type event_type = NXS_EventType_Shared);
        Library createLibrary(void *libraryData, size_t librarySize);
        Library createLibrary(const std::string &libraryPath);
        Buffer createBuffer(size_t _sz, const void *_hostData = nullptr);
        Buffer copyBuffer(Buffer buf);
    };

    typedef Objects<Device> Devices;
}
```

**Methods:**
- `getInfo()`: Get device information
- `createSchedule()`: Create a new command schedule
- `createStream()`: Create a command stream
- `createEvent(nxs_event_type event_type)`: Create an event for synchronization
- `getEvents()`: Get all events associated with this device
- `createLibrary(void *libraryData, size_t librarySize)`: Create library from binary data
- `createLibrary(const std::string &libraryPath)`: Create library from file
- `createBuffer(size_t sz, const void *hostData)`: Create device buffer
- `copyBuffer(Buffer buf)`: Copy buffer to this device

#### Event

Synchronization primitive for coordinating execution between host and device.

```cpp
namespace nexus {
    class Event : public Object<detail::EventImpl> {
    public:
        Event(detail::Impl owner, nxs_int value = 1);
        using Object::Object;

        nxs_int getId() const override;
        std::optional<Property> getProperty(nxs_int prop) const override;

        nxs_status signal(nxs_int value = 1);
        nxs_status wait(nxs_int value = 1);
    };

    typedef Objects<Event> Events;
}
```

**Methods:**
- `signal(nxs_int value)`: Signal the event with a specific value
- `wait(nxs_int value)`: Wait for the event to be signaled with a specific value
- `getProperty(nxs_int prop)`: Get event properties

#### Buffer

Memory buffer for data transfer between host and device.

```cpp
namespace nexus {
    class Buffer : public Object<detail::BufferImpl, detail::SystemImpl> {
    public:
        Buffer(detail::Impl base, size_t _sz, const void *_hostData = nullptr);
        Buffer(detail::Impl base, nxs_int devId, size_t _sz, const void *_hostData = nullptr);
        using Object::Object;

        nxs_int getId() const override;
        nxs_int getDeviceId() const;
        std::optional<Property> getProperty(nxs_int prop) const override;

        size_t getSize() const;
        const char *getData() const;
        Buffer getLocal() const;
        nxs_status copy(void *_hostBuf);
    };

    typedef Objects<Buffer> Buffers;
}
```

**Methods:**
- `getSize()`: Get buffer size in bytes
- `getData()`: Get pointer to buffer data
- `getLocal()`: Get local copy of buffer
- `copy(void *hostBuf)`: Copy data to host buffer

#### Library

Container for compiled kernels and functions.

```cpp
namespace nexus {
    class Library : public Object<detail::LibraryImpl, detail::DeviceImpl> {
    public:
        Library(detail::Impl owner);
        using Object::Object;

        nxs_int getId() const override;
        std::optional<Property> getProperty(nxs_int prop) const override;
        Kernel getKernel(const std::string &kernelName);
    };

    typedef Objects<Library> Librarys;
}
```

**Methods:**
- `getKernel(const std::string &kernelName)`: Get kernel by name

#### Kernel

Represents a compiled kernel function.

```cpp
namespace nexus {
    class Kernel : public Object<detail::KernelImpl, detail::LibraryImpl> {
    public:
        Kernel(detail::Impl owner, const std::string &kernelName);
        using Object::Object;

        nxs_int getId() const override;
        std::optional<Property> getProperty(nxs_int prop) const override;
    };

    typedef Objects<Kernel> Kernels;
}
```

#### Schedule

Command schedule for organizing and executing commands.

```cpp
namespace nexus {
    class Schedule : public Object<detail::ScheduleImpl, detail::DeviceImpl> {
    public:
        Schedule(detail::Impl owner);
        using Object::Object;

        nxs_int getId() const override;
        std::optional<Property> getProperty(nxs_int prop) const override;

        Command createCommand(Kernel kern);
        Command createSignalCommand(nxs_int signal_value = 1);
        Command createSignalCommand(Event event, nxs_int signal_value = 1);
        Command createWaitCommand(Event event, nxs_int wait_value = 1);
        nxs_status run(Stream stream = Stream(), nxs_bool blocking = true);
    };

    typedef Objects<Schedule> Schedules;
}
```

**Methods:**
- `createCommand(Kernel kern)`: Create a new command for the kernel
- `createSignalCommand(nxs_int signal_value)`: Create a signal command with default event
- `createSignalCommand(Event event, nxs_int signal_value)`: Create a signal command for specific event
- `createWaitCommand(Event event, nxs_int wait_value)`: Create a wait command for an event
- `run(Stream stream, nxs_bool blocking)`: Execute the schedule on a stream

#### Command

Individual kernel execution command.

```cpp
namespace nexus {
    class Command : public Object<detail::CommandImpl, detail::ScheduleImpl> {
    public:
        Command(detail::Impl owner, Kernel kern);
        Command(detail::Impl owner, Event event);
        using Object::Object;

        nxs_int getId() const override;
        std::optional<Property> getProperty(nxs_int prop) const override;

        Kernel getKernel() const;
        Event getEvent() const;

        nxs_status setArgument(nxs_uint index, Buffer buffer) const;
        nxs_status finalize(nxs_int gridSize, nxs_int groupSize);
    };

    typedef Objects<Command> Commands;
}
```

**Methods:**
- `getKernel()`: Get the associated kernel (for kernel commands)
- `getEvent()`: Get the associated event (for signal/wait commands)
- `setArgument(nxs_uint index, Buffer buffer)`: Set kernel argument
- `finalize(nxs_int gridSize, nxs_int groupSize)`: Finalize command with execution parameters

### Property System

```cpp
namespace nexus {
    using Prop = std::variant<nxs_long, nxs_double, std::string>;
    using PropIntVec = std::vector<nxs_long>;
    using PropFltVec = std::vector<nxs_double>;
    using PropStrVec = std::vector<std::string>;
    using PropVariant = std::variant<Prop, PropStrVec, PropIntVec, PropFltVec>;

    class Property : public PropVariant {
    public:
        using PropVariant::PropVariant;

        template <nxs_property Tnp>
        typename nxsPropertyType<Tnp>::type getValue() const;

        template <typename T>
        T getValue() const;

        template <typename T>
        std::vector<T> getValueVec() const;
    };

    class Properties : public Object<detail::PropertiesImpl> {
    public:
        Properties(const std::string &filepath);
        using Object::Object;

        std::optional<Property> getProperty(const std::string &prop) const;
        std::optional<Property> getProperty(const std::vector<std::string> &path) const;
        std::optional<Property> getProperty(nxs_int prop) const override;
        std::optional<Property> getProperty(const std::vector<nxs_int> &propPath) const;
    };
}
```

### Event Types

Nexus supports three types of events for different synchronization patterns:

```cpp
enum _nxs_event_type {
    NXS_EventType_Shared = 0,  // Shared events for cross-queue synchronization
    NXS_EventType_Signal = 1,  // Signal events for simple completion notifications
    NXS_EventType_Fence = 2,   // Fence events for kernel completion synchronization
};
typedef enum _nxs_event_type nxs_event_type;
```

### Command Types

Commands can be of different types depending on their purpose:

```cpp
enum _nxs_command_type {
    NXS_CommandType_Dispatch = 0,  // Kernel dispatch command
    NXS_CommandType_Signal = 1,    // Signal event command
    NXS_CommandType_Wait = 2,      // Wait for event command
};
typedef enum _nxs_command_type nxs_command_type;
```

### Error Codes

```cpp
enum _nxs_status {
    NXS_Success                              = 0,
    NXS_DeviceNotFound                       = -1,
    NXS_DeviceNotAvailable                   = -2,
    NXS_CompilerNotAvailable                 = -3,
    NXS_MemObjectAllocationFailure           = -4,
    NXS_OutOfResources                       = -5,
    NXS_OutOfHostMemory                      = -6,
    NXS_ProfilingInfoNotAvailable            = -7,
    NXS_MemCopyOverlap                       = -8,
    NXS_ImageFormatMismatch                  = -9,
    NXS_ImageFormatNotSupported              = -10,
    NXS_BuildProgramFailure                  = -11,
    NXS_MapFailure                           = -12,
    NXS_MisalignedSubBufferOffset            = -13,
    NXS_ExecStatusErrorForEventsInWaitList   = -14,
    NXS_CompileProgramFailure                = -15,
    NXS_LinkerNotAvailable                   = -16,
    NXS_LinkProgramFailure                   = -17,
    NXS_DevicePartitionFailed                = -18,
    NXS_KernelArgInfoNotAvailable            = -19,
    NXS_InvalidValue                         = -30,
    NXS_InvalidDeviceType                    = -31,
    NXS_InvalidRuntime                       = -32,
    NXS_InvalidDevice                        = -33,
    NXS_InvalidContext                       = -34,
    NXS_InvalidQueueProperties               = -35,
    NXS_InvalidCommandQueue                  = -36,
    NXS_InvalidHostPtr                       = -37,
    NXS_InvalidMemObject                     = -38,
    NXS_InvalidImageFormatDescriptor         = -39,
    NXS_InvalidImageSize                     = -40,
    NXS_InvalidSampler                       = -41,
    NXS_InvalidBinary                        = -42,
    NXS_InvalidBuildOptions                  = -43,
    NXS_InvalidProgram                       = -44,
    NXS_InvalidProgramExecutable             = -45,
    NXS_InvalidKernelName                    = -46,
    NXS_InvalidKernelDefinition              = -47,
    NXS_InvalidKernel                        = -48,
    NXS_InvalidArgIndex                      = -49,
    NXS_InvalidArgValue                      = -50,
    NXS_InvalidArgSize                       = -51,
    NXS_InvalidKernelArgs                    = -52,
    NXS_InvalidWorkDimension                 = -53,
    NXS_InvalidWorkGroupSize                 = -54,
    NXS_InvalidWorkItemSize                  = -55,
    NXS_InvalidGlobalOffset                  = -56,
    NXS_InvalidEventWaitList                 = -57,
    NXS_InvalidEvent                         = -58,
    NXS_InvalidOperation                     = -59,
    NXS_InvalidGlObject                      = -60,
    NXS_InvalidBufferSize                    = -61,
    NXS_InvalidMipLevel                      = -62,
    NXS_InvalidGlobalWorkSize                = -63,
    NXS_InvalidProperty                      = -64,
    NXS_InvalidImageDescriptor               = -65,
    NXS_InvalidCompilerOptions               = -66,
    NXS_InvalidDeviceQueue                   = -70,
    NXS_InvalidSpecId                        = -71,
    NXS_MaxSizeRestrictionExceeded           = -72,
    NXS_InvalidObject                        = -80
};
```


## Conclusion

Nexus provides a modern, cross-platform GPU computing framework with both C++ and Python APIs. It supports multiple GPU backends and follows familiar OpenCL-like patterns while providing modern C++ interfaces and comprehensive Python bindings.

For more information, see the test examples in the `test/` directory and the plugin implementations in the `plugins/` directory. 