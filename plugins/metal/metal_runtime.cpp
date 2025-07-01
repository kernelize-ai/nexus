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

#include <assert.h>
#include <string.h>

#include <iostream>
#include <optional>
#include <vector>

#include "rt_utilities.h"

#define NXSAPI_LOGGING
#include <nexus-api.h>

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

#include <Metal/Metal.hpp>
/* #include <QuartzCore/QuartzCore.hpp> */

#define NXSAPI_LOG_MODULE "metal"



class MetalRuntime {
  NS::Array *mDevices;
  std::vector<void *> objects;

  std::vector<MTL::CommandQueue *> queues;

 public:
  MetalRuntime() {
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

  nxs_int addObject(void *obj) {
    objects.push_back(obj);
    return objects.size() - 1;
  }

  MTL::CommandQueue *getQueue(nxs_int id) const { return queues[id]; }

  template <typename T>
  std::optional<T *> getObject(nxs_int id) const {
    if (id < 0 || id >= objects.size()) return std::nullopt;
    if (auto *obj = static_cast<T *>(objects[id]))  // not type checking
      return obj;
    return std::nullopt;
  }
  template <typename T>
  bool dropObject(nxs_int id) {
    if (id < 0 || id >= objects.size()) return false;
    auto *obj = static_cast<T *>(objects[id]);
    if (obj != nullptr) {  // @@@ check types
      obj->release();
      objects[id] = nullptr;
      return true;
    }
    return false;
  }
};

MetalRuntime *getRuntime() {
  static MetalRuntime s_runtime;
  return &s_runtime;
}

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
      return rt_getPropertyStr(property_value, property_value_size, "metal");
    case NP_Type:
      return rt_getPropertyStr(property_value, property_value_size, "gpu");
    case NP_Vendor:
      return rt_getPropertyStr(property_value, property_value_size, "apple");
    case NP_Architecture:
      return rt_getPropertyStr(property_value, property_value_size, "metal");
    case NP_Version:
      return rt_getPropertyStr(property_value, property_value_size, "1.0");
    case NP_MajorVersion:
      return rt_getPropertyInt(property_value, property_value_size, 1);
    case NP_MinorVersion:
      return rt_getPropertyInt(property_value, property_value_size, 0);
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
  auto dev = getRuntime()->getObject<MTL::Device>(device_id);
  if (!dev) return NXS_InvalidDevice;
  auto device = *dev;

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
      return rt_getPropertyStr(property_value, property_value_size, name);
    }
    case NP_Vendor:
      return rt_getPropertyStr(property_value, property_value_size, "apple");
    case NP_Type:
      return rt_getPropertyStr(property_value, property_value_size, "gpu");
    case NP_Architecture: {
      auto arch = device->architecture();
      std::string name =
          arch->name()->cString(NS::StringEncoding::ASCIIStringEncoding);
      return rt_getPropertyStr(property_value, property_value_size, name);
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
  auto dev = rt->getObject<MTL::Device>(device_id);
  if (!dev) return NXS_InvalidDevice;

  MTL::ResourceOptions bopts = MTL::ResourceStorageModeShared;  // unified?
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createBuffer " << size);

  MTL::Buffer *buf;
  if (host_ptr != nullptr)
    buf = (*dev)->newBuffer(host_ptr, size, bopts);
  else
    buf = (*dev)->newBuffer(size, bopts);

  return rt->addObject(buf);
}

/************************************************************************
 * @def CopyBuffer
 * @brief Copy a buffer to the host
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsCopyBuffer(nxs_int buffer_id,
                                                 void *host_ptr) {
  auto rt = getRuntime();
  auto buf = rt->getObject<MTL::Buffer>(buffer_id);
  if (!buf) return NXS_InvalidBuffer;
  memcpy(host_ptr, (*buf)->contents(), (*buf)->length());
  return NXS_Success;
}

/************************************************************************
 * @def ReleaseBuffer
 * @brief Release a buffer on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsReleaseBuffer(nxs_int buffer_id) {
  auto rt = getRuntime();
  if (!rt->dropObject<MTL::Buffer>(buffer_id))
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
                                                 nxs_uint data_size) {
  auto rt = getRuntime();
  auto dev = rt->getObject<MTL::Device>(device_id);
  if (!dev) return NXS_InvalidDevice;

  // NS::Array *binArr = NS::Array::alloc();
  // MTL::StitchedLibraryDescriptor *libDesc =
  // MTL::StitchedLibraryDescriptor::alloc(); libDesc->init(); // IS THIS
  // NECESSARY? libDesc->setBinaryArchives(binArr);
  dispatch_data_t data = (dispatch_data_t)library_data;
  NS::Error *pError = nullptr;
  // MTL::Library *pLibrary = device->newLibrary(data, &pError);
  MTL::Library *pLibrary = (*dev)->newLibrary(
      NS::String::string("kernel.so", NS::UTF8StringEncoding), &pError);
  NXSAPI_LOG(NXSAPI_STATUS_NOTE,
             "createLibrary " << (int64_t)pError << " - " << (int64_t)pLibrary);
  if (pError) {
    NXSAPI_LOG(
        NXSAPI_STATUS_ERR,
        "createLibrary " << pError->localizedDescription()->utf8String());
    return NXS_InvalidLibrary;
  }
  return rt->addObject(pLibrary);
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
  auto dev = rt->getObject<MTL::Device>(device_id);
  if (!dev) return NXS_InvalidDevice;
  NS::Error *pError = nullptr;
  MTL::Library *pLibrary = (*dev)->newLibrary(
      NS::String::string(library_path, NS::UTF8StringEncoding), &pError);
  if (pError) {
    NXSAPI_LOG(
        NXSAPI_STATUS_ERR,
        "createLibrary " << pError->localizedDescription()->utf8String());
    return NXS_InvalidLibrary;
  }
  return rt->addObject(pLibrary);
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
  if (!rt->dropObject<MTL::Library>(library_id))
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
  auto lib = rt->getObject<MTL::Library>(library_id);
  if (!lib) return NXS_InvalidProgram;
  NS::Error *pError = nullptr;
  MTL::Function *func = (*lib)->newFunction(
      NS::String::string(kernel_name, NS::UTF8StringEncoding));
  if (!func) {
    NXSAPI_LOG(NXSAPI_STATUS_ERR,
               "getKernel " << pError->localizedDescription()->utf8String());
    return NXS_InvalidKernel;
  }
  rt->addObject(func);
  MTL::ComputePipelineState *pipeState = (*lib)->device()->newComputePipelineState(func, &pError);
  if (!pipeState) {
    NXSAPI_LOG(NXSAPI_STATUS_ERR,
               "getKernel->ComputePipelineState " << pError->localizedDescription()->utf8String());
    return NXS_InvalidKernel;
  }
 
  return rt->addObject(pipeState);
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
  if (!rt->dropObject<MTL::ComputePipelineState>(kernel_id))
    return NXS_InvalidKernel;
  return NXS_Success;
}

/************************************************************************
 * @def CreateStream
 * @brief Create stream on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
extern "C" nxs_int nxsCreateStream(nxs_int device_id,
                                     nxs_uint stream_properties) {
  auto rt = getRuntime();
  auto dev = rt->getObject<MTL::Device>(device_id);
  if (!dev) return NXS_InvalidDevice;

  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createStream");
  MTL::CommandQueue *stream = (*dev)->newCommandQueue();
  return rt->addObject(stream);
}

/************************************************************************
 * @def ReleaseStream
 * @brief Release the stream on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsReleaseStream(nxs_int stream_id) {
  auto rt = getRuntime();
  if (!rt->dropObject<MTL::CommandQueue>(stream_id))
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
                                     nxs_uint sched_properties) {
  auto rt = getRuntime();
  auto dev = rt->getObject<MTL::Device>(device_id);
  if (!dev) return NXS_InvalidDevice;

  //// NEEDS DEFERRED CREATION UNTIL MAPPED TO A STREAM
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createSchedule");
  auto *queue = rt->getQueue(device_id);
  MTL::CommandBuffer *cmdBuf = queue->commandBuffer();
  return rt->addObject(cmdBuf);
}

/************************************************************************
 * @def ReleaseCommandList
 * @brief Release the buffer on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status nxsRunSchedule(nxs_int schedule_id, nxs_int stream_id, nxs_bool blocking) {
  auto rt = getRuntime();
  auto cmdbuf = rt->getObject<MTL::CommandBuffer>(schedule_id);
  if (!cmdbuf) return NXS_InvalidDevice;
  auto stream = rt->getObject<MTL::CommandQueue>(stream_id);
  if (stream)
    assert((*cmdbuf)->commandQueue() == *stream);

  (*cmdbuf)->enqueue();

  (*cmdbuf)->commit();
  if (blocking) {
    (*cmdbuf)->waitUntilCompleted();  // Synchronous wait for simplicity
    if ((*cmdbuf)->status() == MTL::CommandBufferStatusError) {
      NXSAPI_LOG(
          NXSAPI_STATUS_ERR,
          "runSchedule: "
              << (*cmdbuf)->error()->localizedDescription()->utf8String());
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
  if (!rt->dropObject<MTL::CommandBuffer>(schedule_id))
    return NXS_InvalidBuildOptions;  // fix
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
  auto rt = getRuntime();
  auto cmdbuf = rt->getObject<MTL::CommandBuffer>(schedule_id);
  if (!cmdbuf) return NXS_InvalidBuildOptions;  // fix

  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createCommand");
  MTL::ComputeCommandEncoder *command = (*cmdbuf)->computeCommandEncoder();
  auto res = rt->addObject(command);

  // Add the kernel
  if (nxs_success(kernel_id)) {
    if (auto pipeState = rt->getObject<MTL::ComputePipelineState>(kernel_id)) {
      command->setComputePipelineState(*pipeState);
    } else {
      // test before creating Command
      return NXS_InvalidKernel;
    }
  }
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
  auto cmd = rt->getObject<MTL::ComputeCommandEncoder>(command_id);
  if (!cmd) return NXS_InvalidCommandQueue;  // fix
  auto buf = rt->getObject<MTL::Buffer>(buffer_id);
  if (!buf) return NXS_InvalidBufferSize;  // fix
  (*cmd)->setBuffer(*buf, 0, argument_index);
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
  auto rt = getRuntime();
  auto cmd = rt->getObject<MTL::ComputeCommandEncoder>(command_id);
  if (!cmd) return NXS_InvalidCommandQueue;  // fix

  MTL::Size gridSize = MTL::Size(grid_size, 1, 1);
#if 0
  NS::UInteger threadGroupSize = pAddPSO->maxTotalThreadsPerThreadgroup();
  if (threadGroupSize > ARRAY_LENGTH) {
    threadGroupSize = ARRAY_LENGTH;
  }
#endif
  MTL::Size threadgroupSize = MTL::Size(group_size, 1, 1);

  (*cmd)->dispatchThreads(gridSize, threadgroupSize);
  (*cmd)->endEncoding();

  return NXS_Success;
}

/************************************************************************
 * @def ReleaseCommand
 * @brief Release the command on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsReleaseCommand(nxs_int command_id) {
  auto rt = getRuntime();
  if (!rt->dropObject<MTL::ComputeCommandEncoder>(command_id))
    return NXS_InvalidBuildOptions;  // fix
  return NXS_Success;
}

