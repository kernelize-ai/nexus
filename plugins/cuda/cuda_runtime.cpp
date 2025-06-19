
#include <assert.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <optional>

#define NXSAPI_LOGGING
#include <runtime_device.h>
#include <nexus-api.h>

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

#define NXSAPI_LOG_MODULE "cuda"

class CudaRuntime {
  std::vector<void *> objects;


public:

  Devices *cDevices;


  CudaRuntime() {
    Device dev = Device();
  }
  ~CudaRuntime() {
  }

  nxs_int getDeviceCount() const {
    return 0;
  }

  nxs_int addObject(void *obj) {
    objects.push_back(obj);
    return objects.size() - 1;
  }

  /*MTL::CommandQueue *getQueue(nxs_int id) const {
    return queues[id];
  }*/

  template <typename T>
  std::optional<T*> getObject(nxs_int id) const {
    if (id < 0 || id >= objects.size())
      return std::nullopt;
    if (auto *obj = static_cast<T *>(objects[id])) // not type checking
      return obj;
    return std::nullopt;
  }
  template <typename T>
  std::optional<T*> dropObject(nxs_int id) {
    if (id < 0 || id >= objects.size())
      return std::nullopt;
    if (auto *obj = static_cast<T *>(objects[id])) { // @@@ check types
      objects[id] = nullptr;
      return obj;
    }
    return std::nullopt;
  }
};


CudaRuntime *getRuntime() {
  static CudaRuntime s_runtime;
  return &s_runtime;
}

/*
 * Get the Runtime properties
 */ 
extern "C" nxs_status NXS_API_CALL
nxsGetRuntimeProperty(
  nxs_uint runtime_property_id,
  void *property_value,
  size_t* property_value_size
)
{
  auto rt = getRuntime();

  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "getRuntimeProperty " << runtime_property_id);

  /* lookup HIP equivalent */
  /* return value size */
  /* return value */
  switch (runtime_property_id) {
    case NP_Name: {
      const char *name = "metal";
      if (property_value != NULL) {
        strncpy((char*)property_value, name, strlen(name) + 1);
      } else if (property_value_size != NULL) {
        *property_value_size = strlen(name);
      }
      break;
    }
    case NP_Size: {
      nxs_long size = getRuntime()->getDeviceCount();
      auto sz = sizeof(size);
      if (property_value != NULL) {
        if (property_value_size != NULL && *property_value_size != sz)
          return NXS_InvalidProperty; // PropertySize
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

extern "C" nxs_int NXS_API_CALL
nxsGetDeviceCount()
{
  return getRuntime()->getDeviceCount();
}

/*
 * Get the number of supported platforms on this system. 
 * On POCL, this trivially reduces to 1 - POCL itself.
 */ 

extern "C" nxs_status NXS_API_CALL
nxsGetDeviceProperty(
  nxs_int device_id,
  nxs_uint property_id,
  void *property_value,
  size_t* property_value_size
)
{/*
  auto dev = getRuntime()->getObject<MTL::Device>(device_id);
  if (!dev)
    return NXS_InvalidDevice;
  auto device = *dev;

  auto getStr = [&](const char *name, size_t len) {
    if (property_value != NULL) {
      if (property_value_size == NULL)
        return NXS_InvalidArgSize;
      else if (*property_value_size < len)
        return NXS_InvalidArgValue;
      strncpy((char*)property_value, name, len);
    } else if (property_value_size != NULL) {
      *property_value_size = len;
    }
    return NXS_Success;
  };

  switch (property_id) {
    case NP_Name: {
      std::string name = device->name()->cString(NS::StringEncoding::ASCIIStringEncoding);
      return getStr(name.c_str(), name.size()+1);
    }
    case NP_Vendor:
      return getStr("apple", 6);
    case NP_Type:
      return getStr("gpu", 4);
    case NP_Architecture: {
      auto arch = device->architecture();
      std::string name = arch->name()->cString(NS::StringEncoding::ASCIIStringEncoding);
      return getStr(name.c_str(), name.size()+1);
    }

    default:
      return NXS_InvalidProperty;
  }*/
  return NXS_Success;
}

/*
 * Get the number of supported platforms on this system. 
 * On POCL, this trivially reduces to 1 - POCL itself.
 */ 
extern "C" nxs_status NXS_API_CALL
nxsGetDevicePropertyFromPath(
    nxs_int device_id,
    nxs_uint property_path_count,
    nxs_uint *property_id,
    void *property_value,
    size_t* property_value_size
)
{
  if (property_path_count == 1)
    return nxsGetDeviceProperty(device_id, *property_id, property_value, property_value_size);
  switch (property_id[0]) {
    case NP_CoreSubsystem:
      break;
    case NP_MemorySubsystem:
      break;
    default:
      return NXS_InvalidProperty;
  }
  return NXS_Success;
}

/*
 * Allocate a buffer on the device.
 */
/*
extern "C" nxs_int NXS_API_CALL
nxsCreateBuffer(
  nxs_int device_id,
  size_t size,
  nxs_uint mem_flags,
  void* host_ptr
)
{
  auto rt = getRuntime();
  auto dev = rt->getObject<MTL::Device>(device_id);
  if (!dev)
    return NXS_InvalidDevice;

  MTL::ResourceOptions bopts = MTL::ResourceStorageModeShared; // unified?
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createBuffer " << size);

  MTL::Buffer *buf;
  if (host_ptr != nullptr)
    buf = (*dev)->newBuffer(host_ptr, size, bopts);
  else
    buf = (*dev)->newBuffer(size, bopts);

  return rt->addObject(buf);
}
*/
/*
extern "C" nxs_status NXS_API_CALL
nxsCopyBuffer(
  nxs_int buffer_id,
  void* host_ptr
)
{
  auto rt = getRuntime();
  auto buf = rt->dropObject<MTL::Buffer>(buffer_id);
  if (!buf)
    return NXS_InvalidBuildOptions; // fix
  memcpy(host_ptr, (*buf)->contents(), (*buf)->length());
  return NXS_Success;
}
*/
/*
 * Release a buffer on the device.
 */
/*
extern "C" nxs_status NXS_API_CALL
nxsReleaseBuffer(
  nxs_int buffer_id
)
{
  auto rt = getRuntime();
  auto buf = rt->dropObject<MTL::Buffer>(buffer_id);
  if (!buf)
    return NXS_InvalidBuildOptions; // fix

  (*buf)->release();
  return NXS_Success;
}
*/
/*
 * Allocate a buffer on the device.
 */
/*
extern "C" nxs_int NXS_API_CALL
nxsCreateLibrary(
  nxs_int device_id,
  void *library_data,
  nxs_uint data_size
)
{
  auto rt = getRuntime();
  auto dev = rt->getObject<MTL::Device>(device_id);
  if (!dev)
  return NXS_InvalidDevice;

  // NS::Array *binArr = NS::Array::alloc();
  // MTL::StitchedLibraryDescriptor *libDesc = MTL::StitchedLibraryDescriptor::alloc();
  // libDesc->init(); // IS THIS NECESSARY?
  // libDesc->setBinaryArchives(binArr);
  dispatch_data_t data = (dispatch_data_t)library_data;
  NS::Error *pError = nullptr;
  // MTL::Library *pLibrary = device->newLibrary(data, &pError);
  MTL::Library *pLibrary = (*dev)->newLibrary(
    NS::String::string("kernel.so", NS::UTF8StringEncoding), &pError
  );
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createLibrary " << (int64_t)pError << " - " << (int64_t)pLibrary);
  if (pError) {
    NXSAPI_LOG(NXSAPI_STATUS_ERR, "createLibrary " << pError->localizedDescription()->utf8String());
    return NXS_InvalidProgram;
  }
  return rt->addObject(pLibrary);
}
*/
/*
 * Allocate a buffer on the device.
 */
/*
extern "C" nxs_int NXS_API_CALL
nxsCreateLibraryFromFile(
  nxs_int device_id,
  const char *library_path
)
{
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createLibraryFromFile " << device_id << " - " << library_path);
  auto rt = getRuntime();
  auto dev = rt->getObject<MTL::Device>(device_id);
  if (!dev)
    return NXS_InvalidDevice;
  NS::Error *pError = nullptr;
  MTL::Library *pLibrary = (*dev)->newLibrary(
    NS::String::string(library_path, NS::UTF8StringEncoding), &pError
  );
  if (pError) {
    NXSAPI_LOG(NXSAPI_STATUS_ERR, "createLibrary " << pError->localizedDescription()->utf8String());
    return NXS_InvalidProgram;
  }
  return rt->addObject(pLibrary);
}
*/
/*
 * Release a Library.
 */
/*
extern "C" nxs_status NXS_API_CALL
nxsReleaseLibrary(
  nxs_int library_id
)
{
  auto rt = getRuntime();
  auto lib = rt->dropObject<MTL::Library>(library_id);
  if (!lib)
    return NXS_InvalidProgram;
  (*lib)->release();
  return NXS_Success;
}
*/
/*
 * Lookup a Kernel in a Library.
 */
/*
extern "C" nxs_int NXS_API_CALL
nxsGetKernel(
  nxs_int library_id,
  const char *kernel_name
)
{
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "getKernel " << library_id << " - " << kernel_name);
  auto rt = getRuntime();
  auto lib = rt->getObject<MTL::Library>(library_id);
  if (!lib)
    return NXS_InvalidProgram;
  NS::Error *pError = nullptr;
  MTL::Function *func = (*lib)->newFunction(
    NS::String::string(kernel_name, NS::UTF8StringEncoding));
  if (!func) {
    NXSAPI_LOG(NXSAPI_STATUS_ERR, "getKernel " << pError->localizedDescription()->utf8String());
    return NXS_InvalidKernel;
  }
  return rt->addObject(func);
}
*/

 /************************************************************************
 * @def CreateCommandBuffer
 * @brief Create command buffer on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
/*
extern "C" nxs_int nxsCreateSchedule(
  nxs_int device_id,
  nxs_uint sched_properties
)
{
  auto rt = getRuntime();
  auto dev = rt->getObject<MTL::Device>(device_id);
  if (!dev)
    return NXS_InvalidDevice;

  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createSchedule");
  auto *queue = rt->getQueue(device_id);
  MTL::CommandBuffer *cmdBuf = queue->commandBuffer();
  return rt->addObject(cmdBuf);
}
*/
/************************************************************************
* @def ReleaseCommandList
* @brief Release the buffer on the device
* @return Error status or Succes.
***********************************************************************/
/*
extern "C" nxs_status nxsRunSchedule(
  nxs_int schedule_id,
  nxs_bool blocking
)
{
  auto rt = getRuntime();
  auto cmdbuf = rt->getObject<MTL::CommandBuffer>(schedule_id);
  if (!cmdbuf)
    return NXS_InvalidDevice;

  (*cmdbuf)->commit();
  if (blocking) {
    (*cmdbuf)->waitUntilCompleted(); // Synchronous wait for simplicity
    if ((*cmdbuf)->status() == MTL::CommandBufferStatusError) {
      NXSAPI_LOG(NXSAPI_STATUS_ERR, "runSchedule: "
                << (*cmdbuf)->error()->localizedDescription()->utf8String());
      return NXS_InvalidEvent;
    }  
  }
  return NXS_Success;
}
*/

/*
 * Allocate a buffer on the device.
 */ 
/*
extern "C" nxs_status NXS_API_CALL
nxsReleaseSchedule(
  nxs_int schedule_id
)
{
  auto rt = getRuntime();
  auto cmdbuf = rt->dropObject<MTL::CommandBuffer>(schedule_id);
  if (!cmdbuf)
    return NXS_InvalidBuildOptions; // fix

  (*cmdbuf)->release();
  return NXS_Success;
}
*/
/************************************************************************
 * @def CreateCommand
 * @brief Create command buffer on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
/*
extern "C" nxs_int NXS_API_CALL
nxsCreateCommand(
  nxs_int schedule_id,
  nxs_int kernel_id
)
{
  auto rt = getRuntime();
  auto cmdbuf = rt->getObject<MTL::CommandBuffer>(schedule_id);
  if (!cmdbuf)
    return NXS_InvalidBuildOptions; // fix

  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createCommand");
  MTL::ComputeCommandEncoder *command = (*cmdbuf)->computeCommandEncoder();
  auto res = rt->addObject(command);

  // Add the kernel
  if (kernel_id >= 0) {
    NS::Error *pError = nullptr;
    MTL::ComputePipelineState *pipeState = nullptr;
    if (auto kern = rt->getObject<MTL::Function>(kernel_id)) {
      pipeState = (*cmdbuf)->device()->newComputePipelineState(*kern, &pError);
      command->setComputePipelineState(pipeState);
    } else {
      // test before creating Command
      return NXS_InvalidKernel;
    }
  }
  return res;
}
*/
/************************************************************************
 * @def CreateCommand
 * @brief Create command buffer on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
/*
extern "C" nxs_status NXS_API_CALL
nxsSetCommandArgument(
  nxs_int command_id,
  nxs_int argument_index,
  nxs_int buffer_id
)
{
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "setCommandArg " << command_id << " - " << argument_index << " - " << buffer_id);
  auto rt = getRuntime();
  auto cmd = rt->getObject<MTL::ComputeCommandEncoder>(command_id);
  if (!cmd)
    return NXS_InvalidCommandQueue; // fix
  auto buf = rt->getObject<MTL::Buffer>(buffer_id);
    if (!buf)
      return NXS_InvalidBufferSize; // fix
  (*cmd)->setBuffer(*buf, 0, argument_index);
  return NXS_Success;
}
*/
/************************************************************************
 * @def CreateCommand
 * @brief Create command buffer on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
/*
extern "C" nxs_status NXS_API_CALL
nxsFinalizeCommand(
  nxs_int command_id,
  nxs_int group_size,
  nxs_int grid_size
)
{
  auto rt = getRuntime();
  auto cmd = rt->getObject<MTL::ComputeCommandEncoder>(command_id);
  if (!cmd)
    return NXS_InvalidCommandQueue; // fix

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
}*/
