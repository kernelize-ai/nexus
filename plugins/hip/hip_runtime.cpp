/*
 * Nexus HIP Runtime Plugin
 *
 * This file implements the Nexus API for AMD HIP GPU computing.
 * It provides a mapping from the Nexus unified GPU computing API to
 * AMD's HIP (Heterogeneous-Computing Interface for Portability) framework,
 * enabling cross-platform GPU applications to run on AMD GPUs and
 * other HIP-compatible platforms.
 *
 * ====================================================================
 * NEXUS API TO HIP API MAPPING
 * ====================================================================
 *
 * Core Concepts:
 * --------------
 * Nexus Runtime    -> HIP Runtime (managing all HIP devices)
 * Nexus Device     -> hipDevice_t (represents a HIP GPU device)
 * Nexus Buffer     -> hipDeviceptr_t (GPU memory buffer)
 * Nexus Library    -> hipModule_t (compiled HIP module/library)
 * Nexus Kernel     -> hipFunction_t (compiled kernel function)
 * Nexus Stream     -> hipStream_t (asynchronous execution stream)
 * Nexus Schedule   -> HipSchedule (command collection for execution)
 * Nexus Command    -> HipCommand (individual kernel launch command)
 *
 * API Function Mappings:
 * ----------------------
 *
 * Runtime Management:
 * - nxsGetRuntimeProperty() -> Returns HIP runtime properties (name="hip",
 * device count, version)
 *
 * Device Management:
 * - nxsGetDeviceProperty() -> Maps to hipDeviceProp_t properties:
 *   * NP_Name -> hipDeviceGetName()
 *   * NP_Vendor -> "amd" (hardcoded)
 *   * NP_Type -> "gpu" (hardcoded)
 *   * NP_Architecture -> gcnArchName (GCN architecture)
 *   * NP_Features -> GCN features from architecture name
 *
 * Memory Management:
 * - nxsCreateBuffer() -> hipMalloc():
 *   * Allocates device memory
 *   * Supports host pointer initialization via hipMemcpyHostToDevice
 * - nxsCopyBuffer() -> hipMemcpy() with hipMemcpyDeviceToHost
 * - nxsReleaseBuffer() -> hipFree()
 *
 * Kernel Management:
 * - nxsCreateLibrary() -> hipModuleLoadData():
 *   * Loads HIP module from binary data
 *   * Supports both binary data and file-based library creation
 * - nxsCreateLibraryFromFile() -> hipModuleLoad() with file path
 * - nxsGetKernel() -> hipModuleGetFunction():
 *   * Retrieves kernel function from loaded module
 *   * Returns hipFunction_t for kernel execution
 * - nxsReleaseLibrary() -> hipModuleUnload()
 * - nxsReleaseKernel() -> No HIP equivalent (function pointers are static)
 *
 * Execution Management:
 * - nxsCreateStream() -> hipStreamCreate():
 *   * Creates asynchronous execution stream
 * - nxsCreateSchedule() -> HipSchedule object:
 *   * Collects commands for batch execution
 * - nxsCreateCommand() -> HipCommand object:
 *   * Wraps kernel function and parameters
 * - nxsSetCommandArgument() -> Stores buffer pointers in command:
 *   * Arguments are passed to hipModuleLaunchKernel
 * - nxsFinalizeCommand() -> Sets grid and block dimensions:
 *   * Configures kernel launch parameters
 * - nxsRunSchedule() -> hipModuleLaunchKernel() + hipStreamSynchronize():
 *   * Launches kernels on specified stream
 *   * Optionally waits for completion (blocking mode)
 *
 * Resource Management:
 * - All Nexus objects are tracked in a global object registry
 * - Object IDs are used for cross-API object references
 * - Automatic cleanup via RAII and explicit release calls
 *
 * Limitations and Notes:
 * ----------------------
 *
 * 1. Memory Model:
 *    - Uses device memory allocation (hipMalloc)
 *    - Explicit memory transfers between host and device
 *    - No unified memory support (unlike CUDA)
 *
 * 2. Kernel Compilation:
 *    - Libraries are loaded from compiled HIP modules
 *    - No runtime kernel compilation support
 *    - Kernels must be pre-compiled to device code
 *
 * 3. Synchronization:
 *    - Uses stream-based synchronization
 *    - Limited event support (currently disabled)
 *    - Blocking synchronization via hipStreamSynchronize
 *
 * 4. Error Handling:
 *    - HIP error checking with hipGetErrorString
 *    - Error propagation to Nexus API
 *    - Standard HIP error codes
 *
 * 5. Performance Considerations:
 *    - Command batching through HipSchedule
 *    - Stream-based asynchronous execution
 *    - Grid and block size optimization
 *
 * 6. Platform Support:
 *    - AMD GPUs: Full HIP support
 *    - NVIDIA GPUs: Limited to HIP compatibility layer
 *    - Other platforms: As supported by HIP
 *
 * Future Improvements:
 * -------------------
 *
 * 1. Enhanced Memory Management:
 *    - Memory pooling and reuse
 *    - Asynchronous memory transfers
 *    - Pinned memory support
 *
 * 2. Better Kernel Support:
 *    - Runtime kernel compilation
 *    - Kernel specialization
 *    - Dynamic library loading
 *
 * 3. Advanced Synchronization:
 *    - Full event support
 *    - Multi-stream execution
 *    - Dependency tracking
 *
 * 4. Performance Optimizations:
 *    - Kernel fusion
 *    - Memory access pattern optimization
 *    - Stream management optimization
 *
 * 5. Error Handling:
 *    - Comprehensive error reporting
 *    - Error recovery mechanisms
 *    - Debug information
 *
 * ====================================================================
 */

#define NXSAPI_LOGGING

#include <assert.h>
#include <hip/hip_runtime.h>
#include <nexus-api.h>
#include <rt_buffer.h>
#include <rt_runtime.h>
#include <rt_utilities.h>
#include <string.h>

#include <optional>
#include <vector>

#define NXSAPI_LOG_MODULE "hip_runtime"

#define MAX_ARGS 64

#undef NXS_API_CALL
#define NXS_API_CALL __attribute__((visibility("default")))

using namespace nxs;

////////////////////////////////////////////////////////////////////////////
// Print value
////////////////////////////////////////////////////////////////////////////
template <typename T>
std::string print_value(T value) {
  std::stringstream ss;
  ss << " - 0x" << std::hex << (int64_t)value;
  return ss.str();
}

std::string print_value() { return ""; }

template <typename T, typename... Args>
std::string print_value(T value, Args... args) {
  std::stringstream ss;
  ss << " - 0x" << std::hex << (int64_t)value;
  return ss.str() + print_value(args...);
}

////////////////////////////////////////////////////////////////////////////
// HIP CHECK
////////////////////////////////////////////////////////////////////////////
#define HIP_CHECK(err_code, hip_cmd, ...)                                     \
  do {                                                                        \
    NXSAPI_LOG(NXSAPI_STATUS_NOTE,                                            \
               "HIP_CHECK " << #hip_cmd << print_value(__VA_ARGS__));         \
    hipError_t err = hip_cmd(__VA_ARGS__);                                    \
    if (err != hipSuccess) {                                                  \
      NXSAPI_LOG(NXSAPI_STATUS_ERR, "HIP error: " << hipGetErrorString(err)); \
      return err_code;                                                        \
    }                                                                         \
  } while (0)

////////////////////////////////////////////////////////////////////////////
// Hip Runtime
////////////////////////////////////////////////////////////////////////////
class HipRuntime : public rt::Runtime {
  nxs_int count;
  nxs_int current_device;
  std::vector<hipStream_t> streams;

 public:
  HipRuntime() : rt::Runtime() {
    if (hipGetDeviceCount(&count) != hipSuccess) {
      NXSAPI_LOG(NXSAPI_STATUS_ERR, "hipGetDeviceCount failed");
      count = 0;
    }
    for (int i = 0; i < count; ++i) {
      hipDevice_t dev;
      if (hipDeviceGet(&dev, i) == hipSuccess) {
        hipStream_t stream;
        if (hipStreamCreate(&stream) != hipSuccess) continue;
        addObject(dev);
        streams.push_back(stream);
      }
    }
    if (count > 0) {
      current_device = 0;
      if (hipSetDevice(current_device) != hipSuccess)
        NXSAPI_LOG(NXSAPI_STATUS_ERR, "hipSetDevice failed");
    }
  }
  ~HipRuntime() {
    for (auto stream : streams)
      if (hipStreamDestroy(stream) != hipSuccess)
        NXSAPI_LOG(NXSAPI_STATUS_ERR, "hipStreamDestroy failed");
  }

  template <typename T>
  T getPtr(nxs_int id) {
    if (auto obj = get(id)) return static_cast<T>(*obj);
    return nullptr;
  }

  nxs_int getDeviceCount() const { return count; }

  hipStream_t getStream(nxs_int id) const { return streams[id]; }
  hipDevice_t getDevice(nxs_int id) {
    if (id < 0 || id >= count) return -1;
    if (id != current_device) {
      HIP_CHECK(-1, hipSetDevice, id);
      current_device = id;
    }
    return id;
  }
};

HipRuntime *getRuntime() {
  static HipRuntime s_runtime;
  return &s_runtime;
}

////////////////////////////////////////////////////////////////////////////
// Hip Command
////////////////////////////////////////////////////////////////////////////
class HipCommand {
  hipFunction_t kernel;
  nxs_command_type type;
  nxs_int event_value;
  std::vector<void *> args;
  std::vector<void *> args_ref;
  nxs_long block_size;
  nxs_long grid_size;

 public:
  HipCommand(hipFunction_t kernel, nxs_command_type type,
             nxs_int event_value = 0)
      : kernel(kernel),
        type(type),
        event_value(event_value),
        block_size(1),
        grid_size(1),
        args(MAX_ARGS, nullptr),
        args_ref(MAX_ARGS, nullptr) {
    for (int i = 0; i < args.size(); i++) args_ref[i] = &args[i];
  }

  void addArgument(int idx, void *arg) { args[idx] = arg; }

  nxs_status runCommand(HipRuntime *rt, hipStream_t stream) {
    NXSAPI_LOG(NXSAPI_STATUS_NOTE, "runCommand " << kernel << " - " << type);

    switch (type) {
      case NXS_CommandType_Dispatch: {
        int flags = 0;
        HIP_CHECK(NXS_InvalidCommand, hipModuleLaunchKernel, kernel, grid_size,
                  1, 1, block_size, 1, 1, 0, stream, args_ref.data(), nullptr);
        // hipModuleLaunchCooperativeKernel - for inter-block coordination
        // hipModuleLaunchCooperativeKernelMultiDevice
        // hipLaunchKernelGGL - simplified for non-module kernels
        return NXS_Success;
      }
      case NXS_CommandType_Signal: {
#if 0
        auto event = rt->getPtr<hipEvent_t>(id);
        if (!event) return NXS_InvalidEvent;
        //hipEventRecord(*event, stream);
        return NXS_Success;
#endif
        return NXS_Success;
      }
      case NXS_CommandType_Wait: {
#if 0
        auto event = rt->getPtr<hipEvent_t>(id);
        if (!event) return NXS_InvalidEvent;
        //hipEventSynchronize(*event);
        return NXS_Success;
#endif
        return NXS_Success;
      }
      default:
        return NXS_InvalidCommand;
    }
  }
  void setDimensions(nxs_int block_size, nxs_int grid_size) {
    this->block_size = block_size;
    this->grid_size = grid_size;
  }
  void release() {
    // TODO: release the command buffer
  }
};

////////////////////////////////////////////////////////////////////////////
// Metal Schedule
// - Metal does not support running a command_buffer on multiple command queues
// - We need to create a command buffer for each command queue
// - so command buffer creation is deferred until the schedule is run
////////////////////////////////////////////////////////////////////////////
class HipSchedule {
  std::vector<HipCommand *> commands;

 public:
  HipSchedule() { commands.reserve(32); }
  ~HipSchedule() {
    for (auto cmd : commands) delete cmd;
  }

  void addCommand(HipCommand *cmd) { commands.push_back(cmd); }

  nxs_status run(HipRuntime *rt, hipStream_t stream) {
    for (auto cmd : commands) {
      auto status = cmd->runCommand(rt, stream);
      if (!nxs_success(status)) return status;
    }
    return NXS_Success;
  }
  hipEvent_t getEvent(HipRuntime *rt, hipStream_t stream) {
#if 0
    auto *event = hipEventCreate();
    events[stream] = event;
    // Add all the commands to the command buffer
    for (auto cmd_id : sched->getChildren()) {
      auto cobj = rt->getObject(cmd_id);
      if (!cobj) continue;
      auto *cmd = (*cobj)->get<HipCommand>();
      if (!cmd) continue;
      cmd->createCommand(rt, *cobj, stream);
    }
    return event;
#endif
    return nullptr;
  }
};

/************************************************************************
 * @def GetRuntimeProperty
 * @brief Return Runtime properties
 * @return Error status or Succes.
 ************************************************************************/
extern "C" nxs_status NXS_API_CALL
nxsGetRuntimeProperty(nxs_uint runtime_property_id, void *property_value,
                      size_t *property_value_size) {
  auto rt = getRuntime();

  int runtime_version = 0;
  HIP_CHECK(NXS_InvalidProperty, hipRuntimeGetVersion, &runtime_version);

  int major_version = runtime_version / 10000000;
  int minor_version = (runtime_version % 10000000) / 100000;
  int patch_version = runtime_version % 100000;

  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "getRuntimeProperty " << runtime_property_id);
  /* return value size */
  /* return value */
  switch (runtime_property_id) {
    case NP_Name:
      return rt::getPropertyStr(property_value, property_value_size, "hip");
    case NP_Type:
      return rt::getPropertyStr(property_value, property_value_size, "gpu");
    case NP_Vendor:
      return rt::getPropertyStr(property_value, property_value_size, "amd");
    case NP_Version: {
      char version[128];
      snprintf(version, 128, "%d.%d.%d", major_version, minor_version,
               patch_version);
      return rt::getPropertyStr(property_value, property_value_size, version);
    }
    case NP_MajorVersion:
      return rt::getPropertyInt(property_value, property_value_size,
                                major_version);
    case NP_MinorVersion:
      return rt::getPropertyInt(property_value, property_value_size,
                                minor_version);
    case NP_Size: {
      nxs_long size = getRuntime()->getDeviceCount();
      return rt::getPropertyInt(property_value, property_value_size, size);
    }
    default:
      return NXS_InvalidProperty;
  }
  return NXS_Success;
}

/************************************************************************
 * @def GetDeviceProperty
 * @brief Return Device properties
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL
nxsGetDeviceProperty(nxs_int device_id, nxs_uint device_property_id,
                     void *property_value, size_t *property_value_size) {
  auto dev = getRuntime()->getDevice(device_id);
  if (dev < 0) return NXS_InvalidDevice;

  hipDeviceProp_t dev_prop;
  HIP_CHECK(NXS_InvalidDevice, hipGetDeviceProperties, &dev_prop, dev);

  // int compute_mode = dev_prop.computeMode;
  // int max_threads_per_block = dev_prop.maxThreadsPerBlock;
  // int max_threads_per_multiprocessor = dev_prop.maxThreadsPerMultiProcessor;
  // int max_threads_per_block_dim = dev_prop.maxThreadsDim[0];

  switch (device_property_id) {
    case NP_Name: {
      char name[128];
      HIP_CHECK(NXS_InvalidDevice, hipDeviceGetName, name, 128, dev);
      return rt::getPropertyStr(property_value, property_value_size, name);
    }
    case NP_Vendor:
      return rt::getPropertyStr(property_value, property_value_size, "amd");
    case NP_Type:
      return rt::getPropertyStr(property_value, property_value_size, "gpu");
    case NP_Architecture: {
      std::string arch = dev_prop.gcnArchName;
      auto ii = arch.find_last_of(':');
      if (ii != std::string::npos) arch = arch.substr(0, ii);
      return rt::getPropertyStr(property_value, property_value_size, arch);
    }
    case NP_Features: {
      std::string features = dev_prop.gcnArchName;
      auto ii = features.find_last_of(':');
      if (ii != std::string::npos) features = features.substr(ii + 1);
      return rt::getPropertyStr(property_value, property_value_size, features);
    }

    default:
      return NXS_InvalidProperty;
  }
  return NXS_Success;
}

/************************************************************************
 * @def CreateBuffer
 * @brief Create a buffer on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_int NXS_API_CALL nxsCreateBuffer(nxs_int device_id, size_t size,
                                                nxs_uint mem_flags,
                                                void *host_ptr) {
  auto rt = getRuntime();
  auto dev = rt->getDevice(device_id);
  if (dev < 0) return NXS_InvalidDevice;

  hipDeviceptr_t buf;
  HIP_CHECK(NXS_InvalidBuffer, hipMalloc, &buf, size);
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "HIP_RESULT: " << print_value(buf));
  if (host_ptr != nullptr) {
    HIP_CHECK(NXS_InvalidBuffer, hipMemcpy, buf, host_ptr, size,
              hipMemcpyHostToDevice);
  }

  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createBuffer " << print_value(buf));

  rt::Buffer *buffer = new rt::Buffer(size, buf);

  return rt->addObject(buffer, true);
}

/************************************************************************
 * @def CopyBuffer
 * @brief Copy a buffer to the host
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsCopyBuffer(nxs_int buffer_id,
                                                 void *host_ptr) {
  auto rt = getRuntime();
  auto buffer = rt->get<rt::Buffer>(buffer_id);
  if (!buffer) return NXS_InvalidBuffer;
  HIP_CHECK(NXS_InvalidBuffer, hipMemcpy, host_ptr, (*buffer)->data(),
            (*buffer)->size(), hipMemcpyDeviceToHost);
  return NXS_Success;
}

/************************************************************************
 * @def ReleaseBuffer
 * @brief Release a buffer on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsReleaseBuffer(nxs_int buffer_id) {
  auto rt = getRuntime();
  auto buffer = rt->get<rt::Buffer>(buffer_id);
  if (buffer) {
    HIP_CHECK(NXS_InvalidBuffer, hipFree, (*buffer)->data());
  }
  if (!rt->dropObject(buffer_id)) return NXS_InvalidBuffer;
  return NXS_Success;
}

/************************************************************************
 * @def CreateLibrary
 * @brief Create a library on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_int NXS_API_CALL nxsCreateLibrary(nxs_int device_id,
                                                 void *library_data,
                                                 nxs_uint data_size) {
  auto rt = getRuntime();
  auto dev = rt->getDevice(device_id);
  if (dev < 0) return NXS_InvalidDevice;

  hipModule_t module;
  HIP_CHECK(NXS_InvalidLibrary, hipModuleLoadData, &module, library_data);
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createLibrary" << print_value(module));
  return rt->addObject(module, false);
}

/************************************************************************
 * @def CreateLibraryFromFile
 * @brief Create a library from a file
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_int NXS_API_CALL
nxsCreateLibraryFromFile(nxs_int device_id, const char *library_path) {
  NXSAPI_LOG(NXSAPI_STATUS_NOTE,
             "createLibraryFromFile " << device_id << " - " << library_path);
  auto rt = getRuntime();
  auto dev = rt->getDevice(device_id);
  if (dev < 0) return NXS_InvalidDevice;
  hipModule_t module;
  HIP_CHECK(NXS_InvalidLibrary, hipModuleLoad, &module, library_path);
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createLibrary" << print_value(module));
  return rt->addObject(module, false);
}

/************************************************************************
 * @def GetLibraryProperty
 * @brief Return Library properties
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL
nxsGetLibraryProperty(nxs_int library_id, nxs_uint library_property_id,
                      void *property_value, size_t *property_value_size) {
  // NS::String*      label() const;
  // NS::Array*       functionNames() const;
  // MTL::LibraryType type() const;
  // NS::String*      installName() const;
  return NXS_Success;
}

/************************************************************************
 * @def ReleaseLibrary
 * @brief Release a library on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsReleaseLibrary(nxs_int library_id) {
  auto rt = getRuntime();
  auto lib = rt->getPtr<hipModule_t>(library_id);
  if (lib) HIP_CHECK(NXS_InvalidLibrary, hipModuleUnload, lib);
  if (!rt->dropObject(library_id)) return NXS_InvalidLibrary;
  return NXS_Success;
}

/************************************************************************
 * @def GetKernel
 * @brief Lookup a kernel in a library
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_int NXS_API_CALL nxsGetKernel(nxs_int library_id,
                                             const char *kernel_name) {
  NXSAPI_LOG(NXSAPI_STATUS_NOTE,
             "getKernel " << library_id << " - " << kernel_name);
  auto rt = getRuntime();
  auto lib = rt->getPtr<hipModule_t>(library_id);
  if (!lib) return NXS_InvalidProgram;
  hipFunction_t func;
  HIP_CHECK(NXS_InvalidKernel, hipModuleGetFunction, &func, lib, kernel_name);
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "getKernel" << print_value(func));
  return rt->addObject(func, false);
}

/************************************************************************
 * @def GetKernelProperty
 * @brief Return Kernel properties
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL
nxsGetKernelProperty(nxs_int kernel_id, nxs_uint kernel_property_id,
                     void *property_value, size_t *property_value_size) {
  // NS::String*            label() const;
  // MTL::FunctionType      functionType() const;
  // MTL::PatchType         patchType() const;
  // NS::Integer            patchControlPointCount() const;
  // NS::Array*             vertexAttributes() const;
  // NS::Array*             stageInputAttributes() const;
  // NS::String*            name() const;
  // NS::Dictionary*        functionConstantsDictionary() const;
  // MTL::FunctionOptions   options() const;

  return NXS_Success;
}

/************************************************************************
 * @def ReleaseKernel
 * @brief Release a kernel on the device
 * @return Error status or Succes.
 ***********************************************************************/
nxs_status NXS_API_CALL nxsReleaseKernel(nxs_int kernel_id) {
  auto rt = getRuntime();
  if (!rt->dropObject(kernel_id)) return NXS_InvalidKernel;
  return NXS_Success;
}

/************************************************************************
 * @def CreateEvent
 * @brief Create event on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
extern "C" nxs_int NXS_API_CALL nxsCreateEvent(nxs_int device_id,
                                               nxs_event_type event_type) {
  auto rt = getRuntime();
  auto parent = rt->getObject(device_id);
  if (!parent) return NXS_InvalidDevice;
  auto dev = (*parent)->get<hipDevice_t>();
  if (!dev) return NXS_InvalidDevice;
#if 0
  hipEvent_t event = nullptr;
  if (event_type == NXS_EventType_Shared) {
    event = hipEventCreate();
  } else if (event_type == NXS_EventType_Signal) {
    event = hipEventCreate();
  } else if (event_type == NXS_EventType_Fence) {
    //event = dev->newFence();
    return NXS_InvalidEvent;
  }
  return rt->addObject(event, true);
#endif
  return NXS_InvalidEvent;
}
/************************************************************************
 * @def GetEventProperty
 * @brief Return Event properties
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL
nxsGetEventProperty(nxs_int event_id, nxs_uint event_property_id,
                    void *property_value, size_t *property_value_size) {
  return NXS_Success;
}
/************************************************************************
 * @def SignalEvent
 * @brief Signal an event
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsSignalEvent(nxs_int event_id,
                                                  nxs_int signal_value) {
  auto rt = getRuntime();
  auto obj = rt->getObject(event_id);
  if (!obj) return NXS_InvalidEvent;
  auto event = (*obj)->get<hipEvent_t>();
  if (!event) return NXS_InvalidEvent;
  // if (hipEventRecord(*event, stream) != hipSuccess)
  //   return NXS_InvalidEvent;
  return NXS_Success;
}
/************************************************************************
 * @def WaitEvent
 * @brief Wait for an event
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsWaitEvent(nxs_int event_id,
                                                nxs_int wait_value) {
  auto rt = getRuntime();
  auto obj = rt->getObject(event_id);
  if (!obj) return NXS_InvalidEvent;
  auto event = (*obj)->get<hipEvent_t>();
  if (!event) return NXS_InvalidEvent;
  // if (hipEventSynchronize(*event) != hipSuccess)
  //   return NXS_InvalidEvent;
  return NXS_Success;
}
/************************************************************************
 * @def ReleaseEvent
 * @brief Release an event on the device
 * @return Error status or Succes.
 ***********************************************************************/
nxs_status NXS_API_CALL nxsReleaseEvent(nxs_int event_id) {
  auto rt = getRuntime();
  if (!rt->dropObject(event_id)) return NXS_InvalidEvent;
  return NXS_Success;
}

/************************************************************************
 * @def CreateStream
 * @brief Create stream on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
extern "C" nxs_int NXS_API_CALL nxsCreateStream(nxs_int device_id,
                                                nxs_uint stream_properties) {
  auto rt = getRuntime();
  auto dev = rt->getDevice(device_id);
  if (!dev) return NXS_InvalidDevice;

  // TODO: Get the default command queue for the first Stream
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createStream");
  hipStream_t stream;
  HIP_CHECK(NXS_InvalidStream, hipStreamCreate, &stream);
  return rt->addObject(stream, false);
}
/************************************************************************
 * @def GetStreamProperty
 * @brief Return Stream properties
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL
nxsGetStreamProperty(nxs_int stream_id, nxs_uint stream_property_id,
                     void *property_value, size_t *property_value_size) {
  return NXS_Success;
}
/************************************************************************
 * @def ReleaseStream
 * @brief Release the stream on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsReleaseStream(nxs_int stream_id) {
  auto rt = getRuntime();
  if (!rt->dropObject(stream_id)) return NXS_InvalidStream;
  return NXS_Success;
}

/************************************************************************
 * @def CreateSchedule
 * @brief Create schedule on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
extern "C" nxs_int NXS_API_CALL nxsCreateSchedule(nxs_int device_id,
                                                  nxs_uint sched_properties) {
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createSchedule " << device_id);
  auto rt = getRuntime();
  auto dev = rt->getDevice(device_id);
  if (dev < 0) return NXS_InvalidDevice;

  auto *sched = new HipSchedule();
  return rt->addObject(sched, true);
}

/************************************************************************
 * @def ReleaseCommandList
 * @brief Release the buffer on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsRunSchedule(nxs_int schedule_id,
                                                  nxs_int stream_id,
                                                  nxs_bool blocking) {
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "runSchedule " << schedule_id << " - "
                                                << stream_id << " - "
                                                << blocking);
  auto rt = getRuntime();
  auto sched = rt->get<HipSchedule>(schedule_id);
  if (!sched) return NXS_InvalidSchedule;
  auto stream = rt->getPtr<hipStream_t>(stream_id);
  if (!stream)         // get default stream
    stream = nullptr;  // default stream

  auto status = (*sched)->run(rt, stream);
  if (!nxs_success(status)) return status;

  if (blocking) HIP_CHECK(NXS_InvalidCommand, hipStreamSynchronize, stream);
  return NXS_Success;
}

/************************************************************************
 * @def ReleaseSchedule
 * @brief Release the schedule on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsReleaseSchedule(nxs_int schedule_id) {
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "releaseSchedule " << schedule_id);
  auto rt = getRuntime();
  if (!rt->dropObject(schedule_id, rt::delete_fn<HipSchedule>))
    return NXS_InvalidSchedule;
  return NXS_Success;
}

/************************************************************************
 * @def CreateCommand
 * @brief Create command on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
extern "C" nxs_int NXS_API_CALL nxsCreateCommand(nxs_int schedule_id,
                                                 nxs_int kernel_id) {
  NXSAPI_LOG(NXSAPI_STATUS_NOTE,
             "createCommand " << schedule_id << " - " << kernel_id);
  auto rt = getRuntime();
  auto sched = rt->get<HipSchedule>(schedule_id);
  if (!sched) return NXS_InvalidSchedule;
  auto kernel = rt->getPtr<hipFunction_t>(kernel_id);
  if (!kernel) return NXS_InvalidKernel;

  auto *cmd = new HipCommand(kernel, NXS_CommandType_Dispatch);
  (*sched)->addCommand(cmd);
  return rt->addObject(cmd, false);
}

/************************************************************************
 * @def CreateSignalCommand
 * @brief Create signal command on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
extern "C" nxs_int NXS_API_CALL nxsCreateSignalCommand(nxs_int schedule_id,
                                                       nxs_int event_id,
                                                       nxs_int signal_value) {
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createSignalCommand " << schedule_id << " - "
                                                        << event_id << " - "
                                                        << signal_value);
  auto rt = getRuntime();
  auto parent = rt->getObject(schedule_id);
  if (!parent) return NXS_InvalidSchedule;
  auto sched = (*parent)->get<HipSchedule>();
  if (!sched) return NXS_InvalidSchedule;
  auto event = rt->get<hipEvent_t>(event_id);
  if (!event) return NXS_InvalidEvent;

  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createSignalCommand");
  auto *cmd = new HipCommand(nullptr, NXS_CommandType_Signal, signal_value);
  auto res = rt->addObject(cmd, true);
  (*parent)->addChild(res);
  return res;
}
/************************************************************************
 * @def CreateWaitCommand
 * @brief Create wait command on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
extern "C" nxs_int NXS_API_CALL nxsCreateWaitCommand(nxs_int schedule_id,
                                                     nxs_int event_id,
                                                     nxs_int wait_value) {
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createWaitCommand " << schedule_id << " - "
                                                      << event_id << " - "
                                                      << wait_value);
  auto rt = getRuntime();
  auto parent = rt->getObject(schedule_id);
  if (!parent) return NXS_InvalidSchedule;
  auto sched = (*parent)->get<HipSchedule>();
  if (!sched) return NXS_InvalidSchedule;
  auto event = rt->get<hipEvent_t>(event_id);
  if (!event) return NXS_InvalidEvent;

  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createWaitCommand");
  auto *cmd = new HipCommand(nullptr, NXS_CommandType_Wait, wait_value);
  auto res = rt->addObject(cmd, true);
  (*parent)->addChild(res);
  return res;
}

/************************************************************************
 * @def SetCommandArgument
 * @brief Set command argument on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsSetCommandArgument(nxs_int command_id,
                                                         nxs_int argument_index,
                                                         nxs_int buffer_id) {
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "setCommandArg " << command_id << " - "
                                                  << argument_index << " - "
                                                  << buffer_id);
  auto rt = getRuntime();
  auto cmd = rt->get<HipCommand>(command_id);
  if (!cmd) return NXS_InvalidCommand;
  auto buffer = rt->get<rt::Buffer>(buffer_id);
  if (!buffer) return NXS_InvalidBuffer;
  if (argument_index >= MAX_ARGS) return NXS_InvalidCommand;

  (*cmd)->addArgument(argument_index, (*buffer)->data());
  return NXS_Success;
}

/************************************************************************
 * @def FinalizeCommand
 * @brief Finalize command on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsFinalizeCommand(nxs_int command_id,
                                                      nxs_int group_size,
                                                      nxs_int grid_size) {
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "finalizeCommand " << command_id << " - "
                                                    << group_size << " - "
                                                    << grid_size);
  auto rt = getRuntime();
  auto cmd = rt->get<HipCommand>(command_id);
  if (!cmd) return NXS_InvalidCommand;

  (*cmd)->setDimensions(group_size, grid_size);

  return NXS_Success;
}

/************************************************************************
 * @def ReleaseCommand
 * @brief Release the command on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsReleaseCommand(nxs_int command_id) {
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "releaseCommand " << command_id);
  auto rt = getRuntime();
  if (!rt->dropObject(command_id, rt::delete_fn<HipCommand>))
    return NXS_InvalidCommand;
  return NXS_Success;
}
