
#include <assert.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <optional>
#include <fstream>
#include <filesystem>

#include <cuda_runtime.h>
#include <cuda.h>

#include <nexus-api.h>

#include <runtime_device.h>
#include <runtime_library.h>
#include <runtime_kernel.h>
#include <runtime_buffer.h>
#include <runtime_command.h>

#define NXSAPI_LOGGING

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

#define NXSAPI_LOG_MODULE "cuda"

#define CHECK_CU(call) \
    do { \
        CUresult err = call; \
        if (err != CUDA_SUCCESS) { \
            const char* errorStr; \
            cuGetErrorString(err, &errorStr); \
            std::cerr << "CUDA Error: " << errorStr << std::endl; \
            exit(1); \
        } \
    } while(0)

#define CHECK_CUDA(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            std::cerr << "CUDA Runtime Error: " << cudaGetErrorString(err) << std::endl; \
            exit(1); \
        } \
    } while(0)

class CudaRuntime {
  std::vector<void *> objects;

public:

  Devices cDevices;
  Kernels cKernels;
  Buffers cBuffers;

  CudaRuntime() {
    CHECK_CU(cuInit(0));

    NXSAPI_LOG(NXSAPI_STATUS_NOTE, "CUDA Runtime initialized with result: " << cuResult);

    probeCudaDevices();

    if (cDevices.empty()) {
      std::cerr << "No CUDA devices found." << std::endl;
      return;
    }

    CHECK_CUDA(cudaSetDevice(0));
  }

  ~CudaRuntime() = default;

  void probeCudaDevices() {
    int deviceCount;
    cudaGetDeviceCount(&deviceCount);
    
    for (int i = 0; i < deviceCount; i++) {
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, i);

        this->cDevices.emplace_back(prop.name, prop.uuid.bytes, prop.pciBusID);
    }
  }

  nxs_int createLibrary(nxs_int device_id, void *library_data, nxs_uint data_size) {
    auto devLib = cDevices[device_id].createLibrary(library_data, data_size);

    return devLib;
  }

  nxs_int loadBuffer(void *ptr) {
    if (ptr == nullptr)
      return -1;

    cBuffers.emplace_back(ptr);
    return cBuffers.size() - 1;
  }

  nxs_int createCommand(nxs_int schedule_id, nxs_int kernel_id) {
    if (schedule_id >= cDevices[0].schedules.size() || kernel_id >= cDevices[0].libraries.size())
      return -1;

    RuntimeLibrary &library = cDevices[0].libraries[kernel_id];
    RuntimeCommand cmd = RuntimeCommand(library);
    cDevices[0].schedules[schedule_id].commands.push_back(cmd);

    return 0;
  }

  nxs_status setCommandArgument(nxs_int command_id,
                                    nxs_int argument_index,
                                    nxs_int buffer_id) {
    if (command_id < 0 || command_id >= cDevices[0].schedules[0].commands.size())
      return NXS_InvalidKernel;
      
    auto &cmd = cDevices[0].schedules[0].commands[command_id];
    if (argument_index < 0 || argument_index > cmd.buffers.size())
      return NXS_InvalidArgIndex;

    cmd.setArgument(argument_index, cBuffers[buffer_id]);

    return NXS_Success;
  }

  nxs_int getDeviceCount() const {
    return cDevices.size();
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

extern "C" nxs_int NXS_API_CALL nxsCreateBuffer(nxs_int device_id, size_t size,
                                                nxs_uint mem_flags,
                                                void *host_ptr)
{
  auto rt = getRuntime();

  float *d_a = nullptr;

  if (host_ptr != nullptr) {
    CHECK_CUDA(cudaSetDevice(device_id));
    CHECK_CUDA(cudaMalloc(&d_a, size));
    CHECK_CUDA(cudaMemcpy(d_a, host_ptr, size, cudaMemcpyHostToDevice));
  }
  else
    return -1;

  return rt->loadBuffer(d_a);
}


extern "C" nxs_status NXS_API_CALL
nxsCopyBuffer(
  nxs_int buffer_id,
  void* host_ptr
)
{
  auto rt = getRuntime();

  float *d_a = nullptr;
  int n = 1024;

  if (host_ptr == nullptr)
    return NXS_InvalidHostPtr;

  cudaMemcpy(host_ptr, rt->cDevices[0].schedules[0].commands[0].buffers[buffer_id].ptr, 
    n * sizeof(float), cudaMemcpyDeviceToHost);

  return NXS_Success;
}


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

extern "C" nxs_int NXS_API_CALL
nxsCreateLibrary(
  nxs_int device_id,
  void *library_data,
  nxs_uint data_size
)
{
  auto rt = getRuntime();
  return rt->createLibrary(device_id, library_data, data_size);
}

/*
 * Allocate a buffer on the device.
 */

extern "C" nxs_int NXS_API_CALL
nxsCreateLibraryFromFile(
  nxs_int device_id,
  const char *library_path
)
{
  auto rt = getRuntime();

  std::string fName = std::filesystem::path(library_path).stem().string();

  std::ifstream file(library_path);
  if (!file.is_open()) {
      return -1;
  }
  
  std::ostringstream ss;
  ss << file.rdbuf();
  
  std::string s = ss.str();
  auto i = rt->createLibrary(device_id, (void *)s.c_str(), s.size());

  return i;
}

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

extern "C" nxs_status nxsRunSchedule(
  nxs_int schedule_id,
  nxs_int stream_id,
  nxs_bool blocking
)
{
  auto rt = getRuntime();

  return rt->cDevices[0].runSchedule(schedule_id, blocking);
}


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

extern "C" nxs_int NXS_API_CALL
nxsCreateCommand(
  nxs_int schedule_id,
  nxs_int kernel_id
)
{
  auto rt = getRuntime();

  nxs_int i = rt->createCommand(schedule_id, 0);

  return i;
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
  auto rt = getRuntime();

  return rt->setCommandArgument(command_id, argument_index, buffer_id);

}
/************************************************************************
 * @def CreateCommand
 * @brief Create command buffer on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/

extern "C" nxs_status NXS_API_CALL
nxsFinalizeCommand(
  nxs_int command_id,
  nxs_int group_size,
  nxs_int grid_size
)
{
  auto rt = getRuntime();

  if (command_id >= rt->cDevices[0].schedules[0].commands.size())
    return NXS_InvalidCommand;

  RuntimeCommand &cmd = rt->cDevices[0].schedules[0].commands[command_id];

  cmd.gridSize = grid_size;
  cmd.blockSize = group_size;

  return NXS_Success;
}

/************************************************************************
 * @def Create Schedule
 * @brief Create a schedule for commands on a device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
extern "C" nxs_int nxsCreateSchedule(nxs_int device_id,
                                     nxs_uint sched_properties) {
  auto rt = getRuntime();
  return rt->cDevices[device_id].createSchedule();
}
