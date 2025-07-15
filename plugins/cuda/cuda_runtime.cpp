#define NXSAPI_LOGGING

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

#include <rt_utilities.h>
#include <rt_runtime.h>
#include <rt_object.h>
#include <cuda_library.h>
#include <cuda_kernel.h>
#include <rt_buffer.h>
#include <rt_command.h>

#include <cuda_device.h>

#define NXSAPI_LOG_MODULE "cuda_runtime"

using namespace nxs;

class CudaRuntime : public rt::Runtime {

public:

  nxs_int numDevices;

  CudaRuntime() : rt::Runtime() {
    CHECK_CU(cuInit(0));

    setupCudaDevices();

    if (this->getNumObjects() == 0) {
      NXSAPI_LOG(NXSAPI_STATUS_ERR, "No Cuda devices found.");
      return;
    }

    numDevices = this->getNumObjects();

    NXSAPI_LOG(NXSAPI_STATUS_NOTE, "CUDA Runtime initialized with result: " << cuResult);
  }
  ~CudaRuntime() = default;

  void setupCudaDevices() {
    int deviceCount;
    cudaGetDeviceCount(&deviceCount);
    
    for (int i = 0; i < deviceCount; i++) {
      cudaDeviceProp prop;
      cudaGetDeviceProperties(&prop, i);

      CudaDevice *device = new CudaDevice(prop.name, prop.uuid.bytes, prop.pciBusID, i, this);
      addObject(this, device);
    }
  }

  nxs_int getDeviceCount() const {
    return numDevices;
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
  // if (property_path_count == 1)
  //   return nxsGetDeviceProperty(device_id, *property_id, property_value, property_value_size);
  // switch (property_id[0]) {
  //   case NP_CoreSubsystem:
  //     break;
  //   case NP_MemorySubsystem:
  //     break;
  //   default:
  //     return NXS_InvalidProperty;
  // }
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
  auto deviceObject = rt->getObject(device_id);
  if (!deviceObject || !*deviceObject) return NXS_InvalidDevice;
  
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createBuffer: " << size);

  CudaBuffer *buf = new CudaBuffer(*deviceObject, device_id, size, host_ptr, true);

  return rt->addObject(*deviceObject, buf, true);
}


extern "C" nxs_status NXS_API_CALL
nxsCopyBuffer(
  nxs_int buffer_id,
  void* host_ptr
)
{
  auto rt = getRuntime();

  auto bufferObject = rt->getObject(buffer_id);
  auto buffer = bufferObject ? (*bufferObject)->get<CudaBuffer>() : nullptr;
  if (!buffer)
    return NXS_InvalidBuffer;
  if (!host_ptr)
    return NXS_InvalidHostPtr;

  CHECK_CUDA(cudaMemcpy(host_ptr, buffer->cudaPtr, buffer->size(), cudaMemcpyDeviceToHost));
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

  auto deviceObject = rt->getObject(device_id);
  if (deviceObject) {
    CudaDevice& device = *static_cast<CudaDevice*>(*deviceObject);

    CHECK_CUDA(cudaSetDevice(device.deviceID));

    auto devLib = device.createLibrary(library_data, data_size);
    return devLib;
  }

  return NXS_InvalidLibrary;
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

  std::ifstream file(library_path);
  if (!file.is_open()) {
   std::cout << "Failed to open file\n";
   return NXS_InvalidLibrary;
  }
  
  std::ostringstream ss;
  ss << file.rdbuf();
  std::string s = ss.str();

  auto deviceObject = rt->getObject(device_id);
  auto device = deviceObject ? (*deviceObject)->get<CudaDevice>() : nullptr;
  if (!device)
   return NXS_InvalidDevice;

  CHECK_CUDA(cudaSetDevice(device->deviceID));

  auto result = device->createLibrary((void *)s.c_str(), s.size());  
  return result;
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
extern "C" nxs_int NXS_API_CALL
nxsGetKernel(nxs_int library_id, const char *kernel_name) {
  auto rt = getRuntime();

  auto libObj = rt->getObject(library_id);
  if (!libObj.has_value()) {
    std::cout << "Library object not found\n";
    return NXS_InvalidKernel;
  }

  auto library = libObj.value()->get<CudaLibrary>();
  if (!library) {
    std::cout << "Cast to CudaLibrary failed or obj field is null\n";
    return NXS_InvalidKernel;
  }

  auto kernel = library->createKernel(kernel_name);

  return rt->addObject(library, kernel, false);
}


 /************************************************************************
 * @def CreateCommandBuffer
 * @brief Create command buffer on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
extern "C" nxs_int nxsCreateSchedule(
  nxs_int device_id,
  nxs_uint sched_properties
)
{
  auto rt = getRuntime();

  auto deviceObject = rt->getObject(device_id);
  auto dev = deviceObject ? (*deviceObject)->get<CudaDevice>() : nullptr;
  if (!dev)
   return NXS_InvalidDevice;

  CudaSchedule *schedule = new CudaSchedule(dev);
  return rt->addObject(dev, schedule, true);
}

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

  auto scheduleObject = rt->getObject(schedule_id);
  auto schedule = scheduleObject ? (*scheduleObject)->get<CudaSchedule>() : nullptr;
  if (!schedule)
    return NXS_InvalidSchedule;

  auto device = (CudaDevice *)schedule->getParent();

  CHECK_CUDA(cudaSetDevice(device->deviceID));

  device->runSchedule(schedule);

  return NXS_Success;
}

/************************************************************************
 * @def CreateCommand
 * @brief Create command buffer on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
extern "C" nxs_int NXS_API_CALL
nxsCreateCommand(nxs_int schedule_id, nxs_int kernel_id) {
  auto rt = getRuntime();

  auto scheduleObject = rt->getObject(schedule_id);
  auto schedule = scheduleObject ? (*scheduleObject)->get<CudaSchedule>() : nullptr;
  if (!schedule)
      return NXS_InvalidSchedule;

  auto kernelObject = rt->getObject(kernel_id);
  auto kernel = kernelObject ? (*kernelObject)->get<CudaKernel>() : nullptr;
  if (!kernel) {
    std::cout << "Failed to get kernel object with ID: " << kernel_id << std::endl;
    return NXS_InvalidKernel;
  }

  CudaCommand *command = new CudaCommand(kernel);
  auto ret = rt->addObject(schedule, command, true);
  if (ret)
    schedule->insertCommand(command);

  return ret;
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

  auto commandObject = rt->getObject(command_id);
  auto command = commandObject ? (*commandObject)->get<CudaCommand>() : nullptr;
  if (!command)
    return NXS_InvalidCommand;

  auto bufferObject = rt->getObject(buffer_id);
  auto buffer = bufferObject ? (*bufferObject)->get<CudaBuffer>() : nullptr;
  if (!buffer)
    return NXS_InvalidArgIndex;

  return command->setArgument(argument_index, buffer);
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

  auto commandObject = rt->getObject(command_id);
  auto command = commandObject ? (*commandObject)->get<CudaCommand>() : nullptr;
  if (!command)
    return NXS_InvalidCommand;

  return command->finalize(grid_size, group_size);
}
