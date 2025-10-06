/*
 * Nexus Metal Runtime Plugin
 * 
 * This file implements the Nexus API for Apple Metal GPU computing.
 * It provides a mapping from the Nexus unified GPU computing API to
 * Apple's Metal framework, enabling cross-platform GPU applications
 * to run on macOS and iOS devices with Metal-capable GPUs.
 * 
 * ====================================================================
 * NEXUS API TO METAL API MAPPING
 * ====================================================================
 * 
 * Core Concepts:
 * --------------
 * Nexus Runtime    -> Metal Runtime (singleton managing all Metal devices)
 * Nexus Device     -> MTL::Device (represents a Metal GPU device)
 * Nexus Buffer     -> MTL::Buffer (GPU memory buffer)
 * Nexus Library    -> MTL::Library (compiled Metal shader library)
 * Nexus Kernel     -> MTL::ComputePipelineState (compiled compute pipeline)
 * Nexus Stream     -> MTL::CommandQueue (command submission queue)
 * Nexus Schedule   -> MTL::CommandBuffer (command buffer for execution)
 * Nexus Command    -> MTL::ComputeCommandEncoder (compute command encoder)
 * 
 * API Function Mappings:
 * ----------------------
 * 
 * Runtime Management:
 * - nxsGetRuntimeProperty() -> Returns Metal runtime properties (name="metal", device count)
 * 
 * Device Management:
 * - nxsGetDeviceProperty() -> Maps to MTL::Device properties:
 *   * NP_Name -> device->name()
 *   * NP_Vendor -> "apple" (hardcoded)
 *   * NP_Type -> "gpu" (hardcoded)
 *   * NP_Architecture -> device->architecture()->name()
 * 
 * Memory Management:
 * - nxsCreateBuffer() -> MTL::Device::newBuffer():
 *   * Uses MTL::ResourceStorageModeShared for unified memory
 *   * Supports both zero-initialized and host-pointer-initialized buffers
 * - nxsCopyBuffer() -> memcpy() from buffer contents to host pointer
 * - nxsReleaseBuffer() -> MTL::Buffer::release()
 * 
 * Kernel Management:
 * - nxsCreateLibrary() -> MTL::Device::newLibrary():
 *   * Currently hardcoded to load "kernel.so" (needs improvement)
 *   * Supports both binary data and file-based library creation
 * - nxsCreateLibraryFromFile() -> MTL::Device::newLibrary() with file path
 * - nxsGetKernel() -> MTL::Library::newFunction() + MTL::Device::newComputePipelineState():
 *   * Creates MTL::Function from library
 *   * Compiles to MTL::ComputePipelineState for execution
 * - nxsReleaseLibrary() -> MTL::Library::release()
 * - nxsReleaseKernel() -> MTL::ComputePipelineState::release()
 * 
 * Execution Management:
 * - nxsCreateStream() -> MTL::Device::newCommandQueue():
 *   * Creates command queue for asynchronous execution
 * - nxsCreateSchedule() -> MTL::CommandQueue::commandBuffer():
 *   * Creates command buffer for command recording
 * - nxsCreateCommand() -> MTL::CommandBuffer::computeCommandEncoder():
 *   * Creates compute command encoder for kernel execution
 * - nxsSetCommandArgument() -> MTL::ComputeCommandEncoder::setBuffer():
 *   * Binds buffer to kernel argument slot
 * - nxsFinalizeCommand() -> MTL::ComputeCommandEncoder::dispatchThreads() + endEncoding():
 *   * Dispatches compute work with threadgroup and grid sizes
 *   * Ends command encoding
 * - nxsRunSchedule() -> MTL::CommandBuffer::commit() + waitUntilCompleted():
 *   * Commits command buffer to queue
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
 *    - Uses MTL::ResourceStorageModeShared for unified memory access
 *    - All buffers are accessible from both CPU and GPU
 *    - No support for device-only memory (MTL::ResourceStorageModePrivate)
 * 
 * 2. Kernel Compilation:
 *    - Libraries are loaded from files (not binary data)
 *    - Kernel compilation is not supported
 * 
 * 3. Synchronization:
 *    - Uses blocking synchronization for simplicity
 *    - No support for events or complex synchronization primitives
 *    - All operations are serialized through command queues
 * 
 * 4. Error Handling:
 *    - Basic error checking with Metal error objects
 *    - Limited error propagation to Nexus API
 *    - Some error codes may not map directly
 * 
 * 5. Performance Considerations:
 *    - Command buffer creation is deferred until execution
 *    - No command buffer reuse or optimization
 *    - Threadgroup size optimization is limited
 * 
 * 6. Platform Support:
 *    - macOS: Full Metal support
 *    - iOS: Limited to available Metal features
 *    - No support for other Apple platforms (tvOS, watchOS)
 * 
 * Future Improvements:
 * -------------------
 * 
 * 1. Enhanced Memory Management:
 *    - Support for device-only memory
 *    - Memory pooling and reuse
 *    - Asynchronous memory transfers
 * 
 * 2. Better Kernel Support:
 *    - Binary library loading
 *    - Kernel specialization
 *    - Dynamic library loading
 * 
 * 3. Advanced Synchronization:
 *    - Event-based synchronization
 *    - Multi-queue execution
 *    - Dependency tracking
 * 
 * 4. Performance Optimizations:
 *    - Command buffer reuse
 *    - Threadgroup size optimization
 *    - Memory access pattern optimization
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
#include <rt_runtime.h>
#include <rt_utilities.h>
#include <string.h>

#include <iostream>
#include <optional>
#include <vector>

#include <nexus-api.h>

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

#include <Metal/Metal.hpp>
/* #include <QuartzCore/QuartzCore.hpp> */

#define NXSAPI_LOG_MODULE "metal"

using namespace nxs;

template <typename T>
void release_fn(void *obj) {
  static_cast<T *>(obj)->release();
}

class MetalRuntime : public rt::Runtime {
  NS::Array *mDevices;
  std::vector<MTL::CommandQueue *> queues;

 public:
  MetalRuntime() : rt::Runtime() {
    mDevices = MTL::CopyAllDevices();
    for (int i = 0; i < mDevices->count(); ++i) {
      auto *dev = mDevices->object<MTL::Device>(i);
      queues.push_back(dev->newCommandQueue());
      addObject(dev);
    }
  }
  ~MetalRuntime() {
    for (auto *queue : queues)
      queue->release();
    mDevices->release();
  }

  nxs_int getDeviceCount() const { return mDevices->count(); }

  MTL::CommandQueue *getQueue(nxs_int id) const { return queues[id]; }
};

MetalRuntime *getRuntime() {
  static MetalRuntime s_runtime;
  return &s_runtime;
}

////////////////////////////////////////////////////////////////////////////
// Metal Command
////////////////////////////////////////////////////////////////////////////
class MetalCommand {
  nxs_int id;
  nxs_command_type type;
  nxs_int event_value;
  nxs_int group_size;
  nxs_int grid_size;
  nxs_uint shared_memory_size;
 public:
  MetalCommand(nxs_int id, nxs_command_type type, nxs_int event_value = 0)
   : id(id), type(type), event_value(event_value), group_size(1), grid_size(1) {}
  nxs_status createCommand(MetalRuntime *rt, rt::Object *cobj, MTL::CommandBuffer *cmdbuf) {
    NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createCommand " << id << " - " << type);

    switch (type) {
      case NXS_CommandType_Dispatch: {
        auto kernel = rt->get<MTL::ComputePipelineState>(id);
        if (!kernel) return NXS_InvalidKernel;
        auto *command = cmdbuf->computeCommandEncoder();
        command->setComputePipelineState(kernel);
        int idx = 0;
        for (auto arg : cobj->getChildren()) {
          auto buf = rt->get<MTL::Buffer>(arg);
          if (!buf) return NXS_InvalidBuffer;
          command->setBuffer(buf, 0, idx++);
        }
        command->dispatchThreads(MTL::Size(grid_size * group_size, 1, 1),
                                 MTL::Size(group_size, 1, 1));
        command->endEncoding();
        return NXS_Success;
      }
      case NXS_CommandType_Signal: {
        auto event = rt->get<MTL::Event>(id);
        if (!event) return NXS_InvalidEvent;
        cmdbuf->encodeSignalEvent(event, event_value);
        return NXS_Success;
      }
      case NXS_CommandType_Wait: {
        auto event = rt->get<MTL::Event>(id);
        if (!event) return NXS_InvalidEvent;
        cmdbuf->encodeWait(event, event_value);
        return NXS_Success;
      }
      default:
        return NXS_InvalidCommand;
    }
  }
  void setDimensions(nxs_int grid_size, nxs_int group_size, nxs_uint shared_memory_size) {
    this->grid_size = grid_size;
    this->group_size = group_size;
    this->shared_memory_size = shared_memory_size;
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
class MetalSchedule {
  std::unordered_map<MTL::CommandQueue *, MTL::CommandBuffer *> cmdbufs;
 public:
  MetalSchedule() {
  }
  ~MetalSchedule() {
    for (auto cmdbuf : cmdbufs) {
      cmdbuf.second->release();
    }
    cmdbufs.clear();
  }

  MTL::CommandBuffer *getCommandBuffer(MetalRuntime *rt, rt::Object *sched, MTL::CommandQueue *queue) {
    auto ii = cmdbufs.find(queue);
    if (ii != cmdbufs.end())
      return ii->second;

    auto *cmdbuf = queue->commandBuffer();
    cmdbufs[queue] = cmdbuf;
    // Add all the commands to the command buffer
    for (auto cmd_id : sched->getChildren()) {
      auto cobj = rt->getObject(cmd_id);
      if (!cobj) continue;
      auto *cmd = (*cobj)->get<MetalCommand>();
      if (!cmd) continue;
      cmd->createCommand(rt, *cobj, cmdbuf);
    }
    return cmdbuf;
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

  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "getRuntimeProperty " << runtime_property_id);

  /* return value size */
  /* return value */
  switch (runtime_property_id) {
    case NP_Name:
      return rt::getPropertyStr(property_value, property_value_size, "metal");
    case NP_Type:
      return rt::getPropertyStr(property_value, property_value_size, "gpu");
    case NP_Vendor:
      return rt::getPropertyStr(property_value, property_value_size, "apple");
    case NP_Architecture:
      return rt::getPropertyStr(property_value, property_value_size, "metal");
    case NP_Version:
      return rt::getPropertyStr(property_value, property_value_size, "1.0");
    case NP_MajorVersion:
      return rt::getPropertyInt(property_value, property_value_size, 1);
    case NP_MinorVersion:
      return rt::getPropertyInt(property_value, property_value_size, 0);
    case NP_Size: {
      nxs_long size = getRuntime()->getDeviceCount();
      auto sz = sizeof(size);
      if (property_value != NULL) {
        if (property_value_size != NULL && *property_value_size != sz)
          return NXS_InvalidProperty;  // PropertySize
        memcpy(property_value, &size, sz);
      } else if (property_value_size != NULL)
        *property_value_size = sz;
      break;
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
  auto device = getRuntime()->get<MTL::Device>(device_id);
  if (!device) return NXS_InvalidDevice;

  //    uint64_t                        registryID() const;
  //     MTL::Size                       maxThreadsPerThreadgroup() const;
  // bool                            lowPower() const;
  // bool                            headless() const;
  // bool                            removable() const;
  // bool                            hasUnifiedMemory() const;
  // uint64_t                        recommendedMaxWorkingSetSize() const;
  // MTL::DeviceLocation             location() const;
  // NS::UInteger                    locationNumber() const;
  // uint64_t                        maxTransferRate() const;
  // bool                            depth24Stencil8PixelFormatSupported() const;
  // MTL::ReadWriteTextureTier       readWriteTextureSupport() const;
  // MTL::ArgumentBuffersTier        argumentBuffersSupport() const;
  // bool                            rasterOrderGroupsSupported() const;
  // bool                            supports32BitFloatFiltering() const;
  // bool                            supports32BitMSAA() const;
  // bool                            supportsQueryTextureLOD() const;
  // bool                            supportsBCTextureCompression() const;
  // bool                            supportsPullModelInterpolation() const;
  // bool                            barycentricCoordsSupported() const;
  // bool                            supportsShaderBarycentricCoordinates() const;
  // NS::UInteger                    currentAllocatedSize() const;
  // bool                            supportsFeatureSet(MTL::FeatureSet featureSet);
  // bool                            supportsFamily(MTL::GPUFamily gpuFamily);
  // bool                            supportsTextureSampleCount(NS::UInteger sampleCount);
  // NS::UInteger                    minimumLinearTextureAlignmentForPixelFormat(MTL::PixelFormat format);
  // NS::UInteger                    minimumTextureBufferAlignmentForPixelFormat(MTL::PixelFormat format);
  // NS::UInteger                    maxThreadgroupMemoryLength() const;
  // NS::UInteger                    maxArgumentBufferSamplerCount() const;
  // bool                            programmableSamplePositionsSupported() const;
  // bool                            supportsRasterizationRateMap(NS::UInteger layerCount);
  // uint64_t                        peerGroupID() const;
  // uint32_t                        peerIndex() const;
  // uint32_t                        peerCount() const;
  // NS::UInteger                    sparseTileSizeInBytes() const;
  // NS::UInteger                    sparseTileSizeInBytes(MTL::SparsePageSize sparsePageSize);
  // MTL::Size                       sparseTileSize(MTL::TextureType textureType, MTL::PixelFormat pixelFormat, NS::UInteger sampleCount, MTL::SparsePageSize sparsePageSize);
  // NS::UInteger                    maxBufferLength() const;
  // NS::Array*                      counterSets() const;
  // bool                            supportsCounterSampling(MTL::CounterSamplingPoint samplingPoint);
  // bool                            supportsVertexAmplificationCount(NS::UInteger count);
  // bool                            supportsDynamicLibraries() const;
  // bool                            supportsRenderDynamicLibraries() const;
  // bool                            supportsRaytracing() const;
  // MTL::SizeAndAlign               heapAccelerationStructureSizeAndAlign(NS::UInteger size);
  // MTL::SizeAndAlign               heapAccelerationStructureSizeAndAlign(const class AccelerationStructureDescriptor* descriptor);
  // bool                            supportsFunctionPointers() const;
  // bool                            supportsFunctionPointersFromRender() const;
  // bool                            supportsRaytracingFromRender() const;
  // bool                            supportsPrimitiveMotionBlur() const;
  // bool                            shouldMaximizeConcurrentCompilation() const;
  // NS::UInteger                    maximumConcurrentCompilationTaskCount() const;


  switch (device_property_id) {
    case NP_Name: {
      std::string name =
          device->name()->cString(NS::StringEncoding::ASCIIStringEncoding);
      return rt::getPropertyStr(property_value, property_value_size, name);
    }
    case NP_Vendor:
      return rt::getPropertyStr(property_value, property_value_size, "apple");
    case NP_Type:
      return rt::getPropertyStr(property_value, property_value_size, "gpu");
    case NP_Architecture: {
      auto arch = device->architecture();
      std::string name =
          arch->name()->cString(NS::StringEncoding::ASCIIStringEncoding);
      return rt::getPropertyStr(property_value, property_value_size, name);
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
                                                void *host_ptr,
                                                nxs_uint settings) {
  auto rt = getRuntime();
  auto parent = rt->getObject(device_id);
  if (!parent) return NXS_InvalidDevice;
  auto dev = (*parent)->get<MTL::Device>();
  if (!dev) return NXS_InvalidDevice;

  MTL::ResourceOptions bopts = MTL::ResourceStorageModeShared;  // unified?
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createBuffer " << size);

  MTL::Buffer *buf;
  if (host_ptr != nullptr)
    buf = dev->newBuffer(host_ptr, size, bopts);
  else
    buf = dev->newBuffer(size, bopts);

  return rt->addObject(buf, true);
}

/************************************************************************
 * @def CopyBuffer
 * @brief Copy a buffer to the host
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsCopyBuffer(nxs_int buffer_id,
                                                 void *host_ptr,
                                                 nxs_uint settings) {
  auto rt = getRuntime();
  auto buf = rt->get<MTL::Buffer>(buffer_id);
  if (!buf) return NXS_InvalidBuffer;
  std::memcpy(host_ptr, buf->contents(), buf->length());
  return NXS_Success;
}

/************************************************************************
 * @def ReleaseBuffer
 * @brief Release a buffer on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsReleaseBuffer(nxs_int buffer_id) {
  auto rt = getRuntime();
  if (!rt->dropObject(buffer_id, release_fn<MTL::Buffer>))
    return NXS_InvalidBuffer;
  return NXS_Success;
}

/************************************************************************
 * @def CreateLibrary
 * @brief Create a library on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_int NXS_API_CALL nxsCreateLibrary(nxs_int device_id,
                                                 void *library_data,
                                                 nxs_uint data_size,
                                                 nxs_uint settings) {
  auto rt = getRuntime();
  auto parent = rt->getObject(device_id);
  if (!parent) return NXS_InvalidDevice;
  auto dev = (*parent)->get<MTL::Device>();
  if (!dev) return NXS_InvalidDevice;

  // NS::Array *binArr = NS::Array::alloc();
  // MTL::StitchedLibraryDescriptor *libDesc =
  // MTL::StitchedLibraryDescriptor::alloc(); libDesc->init(); // IS THIS
  // NECESSARY? libDesc->setBinaryArchives(binArr);
  dispatch_data_t data = (dispatch_data_t)library_data;
  NS::Error *pError = nullptr;
  // MTL::Library *pLibrary = device->newLibrary(data, &pError);
  MTL::Library *pLibrary = dev->newLibrary(
      NS::String::string("kernel.so", NS::UTF8StringEncoding), &pError);
  NXSAPI_LOG(NXSAPI_STATUS_NOTE,
             "createLibrary " << (int64_t)pError << " - " << (int64_t)pLibrary);
  if (pError) {
    NXSAPI_LOG(
        NXSAPI_STATUS_ERR,
        "createLibrary " << pError->localizedDescription()->utf8String());
    return NXS_InvalidLibrary;
  }
  return rt->addObject(pLibrary, true);
}

/************************************************************************
 * @def CreateLibraryFromFile
 * @brief Create a library from a file
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_int NXS_API_CALL nxsCreateLibraryFromFile(
    nxs_int device_id, const char *library_path, nxs_uint settings) {
  NXSAPI_LOG(NXSAPI_STATUS_NOTE,
             "createLibraryFromFile " << device_id << " - " << library_path);
  auto rt = getRuntime();
  auto parent = rt->getObject(device_id);
  if (!parent) return NXS_InvalidDevice;
  auto dev = (*parent)->get<MTL::Device>();
  if (!dev) return NXS_InvalidDevice;
  NS::Error *pError = nullptr;
  MTL::Library *pLibrary = dev->newLibrary(
      NS::String::string(library_path, NS::UTF8StringEncoding), &pError);
  if (pError) {
    NXSAPI_LOG(
        NXSAPI_STATUS_ERR,
        "createLibrary " << pError->localizedDescription()->utf8String());
    return NXS_InvalidLibrary;
  }
  return rt->addObject(pLibrary, true);
}

/************************************************************************
 * @def GetLibraryProperty
 * @brief Return Library properties 
 ***********************************************************************/
extern "C" nxs_status nxsGetLibraryProperty(
  nxs_int library_id,
  nxs_uint library_property_id,
  void *property_value,
  size_t* property_value_size
) {
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
  if (!rt->dropObject(library_id, release_fn<MTL::Library>))
    return NXS_InvalidLibrary;
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
  auto parent = rt->getObject(library_id);
  if (!parent) return NXS_InvalidLibrary;
  auto lib = (*parent)->get<MTL::Library>();
  if (!lib) return NXS_InvalidProgram;
  NS::Error *pError = nullptr;
  MTL::Function *func = lib->newFunction(
      NS::String::string(kernel_name, NS::UTF8StringEncoding));
  if (!func) {
    NXSAPI_LOG(NXSAPI_STATUS_ERR,
               "getKernel " << pError->localizedDescription()->utf8String());
    return NXS_InvalidKernel;
  }
  rt->addObject(func, true);
  MTL::ComputePipelineState *pipeState = lib->device()->newComputePipelineState(func, &pError);
  if (!pipeState) {
    NXSAPI_LOG(NXSAPI_STATUS_ERR,
               "getKernel->ComputePipelineState " << pError->localizedDescription()->utf8String());
    return NXS_InvalidKernel;
  }

  return rt->addObject(pipeState, true);
}

/************************************************************************
 * @def GetKernelProperty
 * @brief Return Kernel properties 
 ***********************************************************************/
extern "C" nxs_status nxsGetKernelProperty(
  nxs_int kernel_id,
  nxs_uint kernel_property_id,
  void *property_value,
  size_t* property_value_size
) {

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
  if (!rt->dropObject(kernel_id, release_fn<MTL::ComputePipelineState>))
    return NXS_InvalidKernel;
  return NXS_Success;
}

/************************************************************************
 * @def CreateEvent
 * @brief Create event on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
extern "C" nxs_int nxsCreateEvent(nxs_int device_id, nxs_event_type event_type,
                                  nxs_uint settings) {
  auto rt = getRuntime();
  auto parent = rt->getObject(device_id);
  if (!parent) return NXS_InvalidDevice;
  auto dev = (*parent)->get<MTL::Device>();
  if (!dev) return NXS_InvalidDevice;

  MTL::Event *event = nullptr;
  if (event_type == NXS_EventType_Shared) {
    event = dev->newSharedEvent();
  } else if (event_type == NXS_EventType_Signal) {
    event = dev->newEvent();
  } else if (event_type == NXS_EventType_Fence) {
    //event = dev->newFence();
    return NXS_InvalidEvent;
  }
  return rt->addObject(event, true);
}
/************************************************************************
 * @def GetEventProperty
 * @brief Return Event properties 
 ***********************************************************************/
extern "C" nxs_status nxsGetEventProperty(
  nxs_int event_id,
  nxs_uint event_property_id,
  void *property_value,
  size_t* property_value_size
) {
  return NXS_Success;
}
/************************************************************************
 * @def SignalEvent
 * @brief Signal an event 
 ***********************************************************************/
extern "C" nxs_status nxsSignalEvent(
  nxs_int event_id,
  nxs_int signal_value
) {
  auto rt = getRuntime();
  auto obj = rt->getObject(event_id);
  if (!obj) return NXS_InvalidEvent;
  auto event = (*obj)->get<MTL::SharedEvent>();
  if (!event) return NXS_InvalidEvent;
  event->setSignaledValue(signal_value);
  return NXS_Success;
}
/************************************************************************
 * @def WaitEvent
 * @brief Wait for an event 
 ***********************************************************************/
extern "C" nxs_status nxsWaitEvent(
  nxs_int event_id,
  nxs_int wait_value
) {
  auto rt = getRuntime();
  auto obj = rt->getObject(event_id);
  if (!obj) return NXS_InvalidEvent;
  auto event = (*obj)->get<MTL::SharedEvent>();
  if (!event) return NXS_InvalidEvent;
  event->waitUntilSignaledValue(wait_value, 1000000000); // 1 second timeout
  return NXS_Success;
}
/************************************************************************
 * @def ReleaseEvent
 * @brief Release an event on the device
 * @return Error status or Succes.
 ***********************************************************************/
 nxs_status NXS_API_CALL nxsReleaseEvent(nxs_int event_id) {
  auto rt = getRuntime();
  if (!rt->dropObject(event_id, release_fn<MTL::SharedEvent>))
    return NXS_InvalidEvent;
  return NXS_Success;
}

/************************************************************************
 * @def CreateStream
 * @brief Create stream on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
extern "C" nxs_int nxsCreateStream(nxs_int device_id,
                                   nxs_uint stream_settings) {
  auto rt = getRuntime();
  auto parent = rt->getObject(device_id);
  if (!parent) return NXS_InvalidDevice;
  auto dev = (*parent)->get<MTL::Device>();
  if (!dev) return NXS_InvalidDevice;

  // TODO: Get the default command queue for the first Stream
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createStream");
  MTL::CommandQueue *queue = dev->newCommandQueue();
  return rt->addObject(queue, true);
}
/************************************************************************
 * @def GetStreamProperty
 * @brief Return Stream properties 
 ***********************************************************************/
extern "C" nxs_status nxsGetStreamProperty(
  nxs_int stream_id,
  nxs_uint stream_property_id,
  void *property_value,
  size_t* property_value_size
) {
  return NXS_Success;
}
/************************************************************************
 * @def ReleaseStream
 * @brief Release the stream on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsReleaseStream(nxs_int stream_id) {
  auto rt = getRuntime();
  if (!rt->dropObject(stream_id, release_fn<MTL::CommandQueue>))
    return NXS_InvalidStream;
  return NXS_Success;
}

/************************************************************************
 * @def CreateSchedule
 * @brief Create schedule on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
extern "C" nxs_int nxsCreateSchedule(nxs_int device_id,
                                     nxs_uint sched_settings) {
  auto rt = getRuntime();
  auto parent = rt->getObject(device_id);
  if (!parent) return NXS_InvalidDevice;
  auto dev = (*parent)->get<MTL::Device>();
  if (!dev) return NXS_InvalidDevice;

  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createSchedule");
  auto *sched = new MetalSchedule();
  return rt->addObject(sched, true);
}

/************************************************************************
 * @def RunSchedule
 * @brief Run the schedule on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status nxsRunSchedule(nxs_int schedule_id, nxs_int stream_id,
                                     nxs_uint sched_settings) {
  auto rt = getRuntime();
  auto parent = rt->getObject(schedule_id);
  if (!parent) return NXS_InvalidSchedule;
  auto sched = (*parent)->get<MetalSchedule>(); 
  if (!sched) return NXS_InvalidSchedule;
  auto stream = rt->get<MTL::CommandQueue>(stream_id);
  if (!stream) return NXS_InvalidStream;

  auto *cmdbuf = sched->getCommandBuffer(rt, *parent, stream);
  if (!cmdbuf) return NXS_InvalidSchedule;

  cmdbuf->enqueue();

  cmdbuf->commit();
  if (!(sched_settings & NXS_ExecutionSettings_NonBlocking)) {
    cmdbuf->waitUntilCompleted();  // Synchronous wait for simplicity
    if (cmdbuf->status() == MTL::CommandBufferStatusError) {
      NXSAPI_LOG(
          NXSAPI_STATUS_ERR,
          "runSchedule: "
              << cmdbuf->error()->localizedDescription()->utf8String());
      return NXS_InvalidEvent;
    }
  }
  return NXS_Success;
}

/************************************************************************
 * @def ReleaseSchedule
 * @brief Release the schedule on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsReleaseSchedule(nxs_int schedule_id) {
  auto rt = getRuntime();
  if (!rt->dropObject(schedule_id, rt::delete_fn<MetalSchedule>))
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
                                                 nxs_int kernel_id,
                                                 nxs_uint command_settings) {
  auto rt = getRuntime();
  auto parent = rt->getObject(schedule_id);
  if (!parent) return NXS_InvalidSchedule;
  auto sched = (*parent)->get<MetalSchedule>();
  if (!sched) return NXS_InvalidSchedule;
  auto pipeState = rt->get<MTL::ComputePipelineState>(kernel_id);
  if (!pipeState) return NXS_InvalidKernel;

  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createCommand");

  auto *cmd = new MetalCommand(kernel_id, NXS_CommandType_Dispatch);
  auto res = rt->addObject(cmd, true);
  (*parent)->addChild(res);
  return res;
}

/************************************************************************
 * @def CreateSignalCommand
 * @brief Create signal command on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
extern "C" nxs_int NXS_API_CALL
nxsCreateSignalCommand(nxs_int schedule_id, nxs_int event_id,
                       nxs_int signal_value, nxs_uint command_settings) {
  auto rt = getRuntime();
  auto parent = rt->getObject(schedule_id);
  if (!parent) return NXS_InvalidSchedule;
  auto sched = (*parent)->get<MetalSchedule>();
  if (!sched) return NXS_InvalidSchedule;
  auto event = rt->get<MTL::Event>(event_id);
  if (!event) return NXS_InvalidEvent;

  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createSignalCommand");
  auto *cmd = new MetalCommand(event_id, NXS_CommandType_Signal, signal_value);
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
extern "C" nxs_int NXS_API_CALL
nxsCreateWaitCommand(nxs_int schedule_id, nxs_int event_id, nxs_int wait_value,
                     nxs_uint command_settings) {
  auto rt = getRuntime();
  auto parent = rt->getObject(schedule_id);
  if (!parent) return NXS_InvalidSchedule;
  auto sched = (*parent)->get<MetalSchedule>();
  if (!sched) return NXS_InvalidSchedule;
  auto event = rt->get<MTL::Event>(event_id);
  if (!event) return NXS_InvalidEvent;

  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createWaitCommand");
  auto *cmd = new MetalCommand(event_id, NXS_CommandType_Wait, wait_value);
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
  auto parent = rt->getObject(command_id);
  if (!parent) return NXS_InvalidCommand;
  auto *cmd = (*parent)->get<MetalCommand>();
  if (!cmd) return NXS_InvalidCommand;
  auto buf = rt->get<MTL::Buffer>(buffer_id);
  if (!buf) return NXS_InvalidBuffer;
  // TODO: needs retained buffer
  (*parent)->addChild(buffer_id, argument_index);
  return NXS_Success;
}

/************************************************************************
 * @def FinalizeCommand
 * @brief Finalize command on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsFinalizeCommand(nxs_int command_id,
                                                      nxs_dim3 grid_size,
                                                      nxs_dim3 group_size,
                                                      nxs_uint shared_memory_size) {
  auto rt = getRuntime();
  auto cmd = rt->get<MetalCommand>(command_id);
  if (!cmd) return NXS_InvalidCommand;

  cmd->setDimensions(grid_size.x, group_size.x, shared_memory_size);

  return NXS_Success;
}
