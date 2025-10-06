#include <cuda_utils.h>

#include <assert.h>
#include <cuda_command.h>
#include <cuda_device.h>
#include <cuda_plugin_runtime.h>
#include <cuda_runtime.h>
#include <rt_buffer.h>
#include <rt_object.h>
#include <rt_utilities.h>
#include <string.h>

#include <nvml.h>

using namespace nxs;

CudaRuntime *getRuntime() {
  static CudaRuntime s_runtime;
  return &s_runtime;
}

/*
 * Get the Runtime properties
 */ 
extern "C" nxs_status NXS_API_CALL
nxsGetRuntimeProperty(nxs_uint runtime_property_id, void *property_value,
                      size_t *property_value_size) {
  auto rt = getRuntime();

  int runtime_version = 0;
  CUDA_CHECK(NXS_InvalidRuntime, cudaRuntimeGetVersion, &runtime_version);

  int major_version = runtime_version / 10000000;
  int minor_version = (runtime_version % 10000000) / 100000;
  int patch_version = runtime_version % 100000;

  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "getRuntimeProperty " << runtime_property_id);
  /* return value size */
  /* return value */
  switch (runtime_property_id) {
    case NP_Keys: {
      nxs_long keys[] = {NP_Name,    NP_Type,         NP_Vendor,
                         NP_Version, NP_MajorVersion, NP_MinorVersion,
                         NP_Size};
      return rt::getPropertyVec(property_value, property_value_size, keys, 7);
    }
    case NP_Name:
      return rt::getPropertyStr(property_value, property_value_size, "cuda");
    case NP_Type:
      return rt::getPropertyStr(property_value, property_value_size, "gpu");
    case NP_Vendor:
      return rt::getPropertyStr(property_value, property_value_size, "NVIDIA");
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

extern "C" nxs_status NXS_API_CALL
nxsGetDeviceProperty(nxs_int device_id, nxs_uint property_id,
                     void *property_value, size_t *property_value_size) {
  auto *rt = getRuntime();
  auto *device = rt->getDevice(device_id);
  if (!device) return NXS_InvalidDevice;

  cudaDeviceProp &props = device->props;
  switch (property_id) {
    case NP_Keys: {
      nxs_long keys[] = {NP_Name,
                         NP_Type,
                         NP_Architecture,
                         NP_MajorVersion,
                         NP_MinorVersion,
                         NP_Size,
                         NP_GlobalMemorySize,
                         NP_CoreMemorySize,
                         NP_CoreRegisterSize,
                         NP_SIMDSize,
                         NP_CoreUtilization,
                         NP_MemoryUtilization};
      return rt::getPropertyVec(property_value, property_value_size, keys, 10);
    }
    case NP_Name:
      return rt::getPropertyStr(property_value, property_value_size, props.name);
    case NP_Type:
      return rt::getPropertyStr(property_value, property_value_size, "gpu");
    case NP_Architecture: {
      std::string name = "sm_" + std::to_string(props.major * 10 + props.minor);
      return rt::getPropertyStr(property_value, property_value_size, name);
    }
    case NP_MajorVersion:
      return rt::getPropertyInt(property_value, property_value_size,
                                props.major);
    case NP_MinorVersion:
      return rt::getPropertyInt(property_value, property_value_size,
                                props.minor);
    case NP_Size:
      return rt::getPropertyInt(property_value, property_value_size,
                                props.multiProcessorCount);
    case NP_GlobalMemorySize:
      return rt::getPropertyInt(property_value, property_value_size,
                                props.totalGlobalMem);
    case NP_CoreMemorySize:
      return rt::getPropertyInt(property_value, property_value_size,
                                props.sharedMemPerBlock);
    case NP_CoreRegisterSize:
      return rt::getPropertyInt(property_value, property_value_size,
                                props.regsPerBlock);
    case NP_SIMDSize:
      return rt::getPropertyInt(property_value, property_value_size,
                                props.warpSize);
    case NP_CoreClockRate:
      return rt::getPropertyInt(property_value, property_value_size,
                                props.clockRate);
    case NP_MemoryClockRate:
      return rt::getPropertyInt(property_value, property_value_size,
                                props.memoryClockRate);
    case NP_MemoryBusWidth:
      return rt::getPropertyInt(property_value, property_value_size,
                                props.memoryBusWidth);
    case NP_CoreUtilization: {
      nvmlDevice_t nvml_device;
      nvmlUtilization_t utilization;
      nxs_long value = -1;

      if (nvmlDeviceGetHandleByIndex(device_id, &nvml_device) != NVML_SUCCESS)
        return NXS_InvalidDevice;

      if (nvmlDeviceGetUtilizationRates(nvml_device, &utilization) != NVML_SUCCESS)
        return NXS_InvalidDevice;

      value = utilization.gpu;

      return rt::getPropertyInt(property_value, property_value_size, value);
    }
    case NP_MemoryUtilization: {
      nvmlDevice_t nvml_device;
      nvmlMemory_t memInfo;
      nxs_long value = -1;

      if (nvmlDeviceGetHandleByIndex(device_id, &nvml_device) != NVML_SUCCESS)
        return NXS_InvalidDevice;

      if (nvmlDeviceGetMemoryInfo(nvml_device, &memInfo) != NVML_SUCCESS)
        return NXS_InvalidDevice;

      value = (memInfo.used * 100) / memInfo.total;  // Memory usage percentage

      return rt::getPropertyInt(property_value, property_value_size, value);
    }
    default:
      return NXS_InvalidProperty;
  }
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
                                                void *data_ptr,
                                                nxs_uint buffer_settings) {
  auto rt = getRuntime();
  auto deviceObject = rt->get<CudaDevice>(device_id);
  if (!deviceObject) return NXS_InvalidDevice;

  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createBuffer: " << size);
  if (!(buffer_settings & NXS_BufferSettings_OnDevice)) {
    void *cuda_ptr = nullptr;
    CUDA_CHECK(NXS_InvalidBuffer, cudaMalloc, &cuda_ptr, size);
    if (data_ptr != nullptr)
      CUDA_CHECK(NXS_InvalidBuffer, cudaMemcpy, cuda_ptr, data_ptr, size,
                 cudaMemcpyHostToDevice);
    data_ptr = cuda_ptr;
  }

  auto *buf = rt->getBuffer(size, data_ptr, false);
  if (!buf) return NXS_InvalidBuffer;

  return rt->addObject(buf);
}

extern "C" nxs_status NXS_API_CALL nxsCopyBuffer(nxs_int buffer_id,
                                                 void *host_ptr,
                                                 nxs_uint copy_settings) {
  auto rt = getRuntime();

  auto buffer = rt->get<rt::Buffer>(buffer_id);
  if (!buffer) return NXS_InvalidBuffer;
  if (!host_ptr) return NXS_InvalidHostPtr;

  CUDA_CHECK(NXS_InvalidBuffer, cudaMemcpy, host_ptr, buffer->get(),
             buffer->size(), cudaMemcpyDeviceToHost);
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
extern "C" nxs_int NXS_API_CALL nxsCreateLibrary(nxs_int device_id,
                                                 void *library_data,
                                                 nxs_uint data_size,
                                                 nxs_uint library_settings) {
  auto rt = getRuntime();

  auto device = rt->getDevice(device_id);
  if (!device) return NXS_InvalidDevice;

  CUmodule module;
  CU_CHECK(NXS_InvalidLibrary, cuModuleLoadData, &module, library_data);

  return rt->addObject(module);
}

/*
 * Allocate a buffer on the device.
 */
extern "C" nxs_int NXS_API_CALL nxsCreateLibraryFromFile(
    nxs_int device_id, const char *library_path, nxs_uint library_settings) {
  auto rt = getRuntime();
  auto device = rt->getDevice(device_id);
  if (!device)
   return NXS_InvalidDevice;

  CUmodule module;
  CU_CHECK(NXS_InvalidLibrary, cuModuleLoad, &module, library_path);

  return rt->addObject(module);
}

/************************************************************************
 * @def GetKernelProperty
 * @brief Return Kernel properties
 ***********************************************************************/
 extern "C" nxs_status NXS_API_CALL nxsGetLibraryProperty(nxs_int library_id,
  nxs_uint library_property_id,
  void *property_value,
  size_t *property_value_size) {
  
  auto rt = getRuntime();
  auto library = rt->getPtr<CUmodule>(library_id);
  if (!library) return NXS_InvalidLibrary;

  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "getLibraryProperty " << library_property_id);

  switch (library_property_id) {
    case NP_Keys: {
      nxs_long keys[] = {NP_Value};
      return rt::getPropertyVec(property_value, property_value_size, keys, 1);
    }
    case NP_Value: {
      return rt::getPropertyInt(property_value, property_value_size,
            (nxs_long)library);
    }
    default:
      return NXS_InvalidProperty;
  }

  return NXS_Success;
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

  auto library = rt->getPtr<CUmodule>(library_id);
  if (!library) return NXS_InvalidLibrary;

  CUfunction kernel = nullptr;
  CUresult result = cuModuleGetFunction(&kernel, library, kernel_name);
  if (result != CUDA_SUCCESS) {
    const char *error_string;
    cuGetErrorString(result, &error_string);
    NXSAPI_LOG(NXSAPI_STATUS_ERR,
               "GetKernel " << kernel_name << " " << error_string);
    return NXS_InvalidKernel;
  }

  return rt->addObject(kernel, false);
}

/************************************************************************
 * @def GetKernelProperty
 * @brief Return Kernel properties
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsGetKernelProperty(nxs_int kernel_id,
                                           nxs_uint kernel_property_id,
                                           void *property_value,
                                           size_t *property_value_size) {
  auto rt = getRuntime();
  auto kernel = rt->getPtr<CUfunction>(kernel_id);
  if (!kernel) return NXS_InvalidKernel;

  switch (kernel_property_id) {
    case NP_Keys: {
      nxs_long keys[] = {NP_Value, NP_CoreRegisterSize, NP_SIMDSize,
                         NP_CoreMemorySize, NP_MaxThreadsPerBlock};
      return rt::getPropertyVec(property_value, property_value_size, keys, 5);
    }
    case NP_Value: {
      return rt::getPropertyInt(property_value, property_value_size,
                                (nxs_long)kernel);
    }
    case NP_CoreRegisterSize: {
      int n_regs = 0;
      CU_CHECK(NXS_InvalidKernel, cuFuncGetAttribute, &n_regs,
               CU_FUNC_ATTRIBUTE_NUM_REGS, kernel);
      return rt::getPropertyInt(property_value, property_value_size, n_regs);
    }
    case NP_SIMDSize: {
      int simd_size = 0;
      CU_CHECK(NXS_InvalidKernel, cuFuncGetAttribute, &simd_size,
               CU_FUNC_ATTRIBUTE_MAX_THREADS_PER_BLOCK, kernel);
      return rt::getPropertyInt(property_value, property_value_size, simd_size);
    }
    case NP_CoreMemorySize: {
      int shared_size = 0;
      CU_CHECK(NXS_InvalidKernel, cuFuncGetAttribute, &shared_size,
               CU_FUNC_ATTRIBUTE_SHARED_SIZE_BYTES, kernel);
      return rt::getPropertyInt(property_value, property_value_size,
                                shared_size);
    }
    case NP_MaxThreadsPerBlock: {
      int max_threads = 0;
      CU_CHECK(NXS_InvalidKernel, cuFuncGetAttribute, &max_threads,
               CU_FUNC_ATTRIBUTE_MAX_THREADS_PER_BLOCK, kernel);
      return rt::getPropertyInt(property_value, property_value_size, max_threads);
    }
    default:
      return NXS_InvalidProperty;
  }

  return NXS_Success;
}

/************************************************************************
 * @def CreateEvent
 * @brief Create event on the device using CUDA Driver API
 ***********************************************************************/
extern "C" nxs_int NXS_API_CALL nxsCreateEvent(nxs_int device_id,
                                               nxs_event_type event_type,
                                               nxs_uint event_settings) {
  auto rt = getRuntime();
  auto parent = rt->getObject(device_id);
  if (!parent) return NXS_InvalidDevice;

  CUevent event;
  if (event_type == NXS_EventType_Shared) {
    CU_CHECK(NXS_InvalidEvent, cuEventCreate, &event, CU_EVENT_DEFAULT);
  } else if (event_type == NXS_EventType_Signal) {
    CU_CHECK(NXS_InvalidEvent, cuEventCreate, &event, CU_EVENT_DISABLE_TIMING);
  } else if (event_type == NXS_EventType_Fence) {
    CU_CHECK(NXS_InvalidEvent, cuEventCreate, &event, CU_EVENT_BLOCKING_SYNC);
  } else {
    return NXS_InvalidEvent; // or whatever error code you use
  }

  return rt->addObject(event);
}

/************************************************************************
 * @def SignalEvent - Record event using Driver API
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsSignalEvent(nxs_int event_id,
                                                  nxs_int signal_value) {
  auto rt = getRuntime();
  auto event = rt->getPtr<CUevent>(event_id);
  if (!event) return NXS_InvalidEvent;
  CU_CHECK(NXS_InvalidEvent, cuEventRecord, event,
           0);  // Remove the * - use event directly
  return NXS_Success;
}

/************************************************************************
 * @def WaitEvent - Synchronize event using Driver API
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsWaitEvent(nxs_int event_id,
                                                nxs_int wait_value) {
  auto rt = getRuntime();
  auto event = rt->getPtr<CUevent>(event_id);
  if (!event) return NXS_InvalidEvent;
  CU_CHECK(NXS_InvalidEvent, cuEventSynchronize,
           event);  // Remove the * - use event directly
  return NXS_Success;
}

/************************************************************************
 * @def ReleaseEvent - Destroy event using Driver API
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsReleaseEvent(nxs_int event_id) {
  auto rt = getRuntime();
  auto event = rt->getPtr<CUevent>(event_id);
  if (!event) return NXS_InvalidEvent;
  CU_CHECK(NXS_InvalidEvent, cuEventDestroy,
           event);  // Remove the * - use event directly
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
                                                nxs_uint stream_settings) {
  auto rt = getRuntime();
  auto device = rt->get<CudaDevice>(device_id);
  if (!device) return NXS_InvalidDevice;

  // TODO: Get the default command queue for the first Stream
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createStream");
  cudaStream_t stream;
  CUDA_CHECK(NXS_InvalidStream, cudaStreamCreate, &stream);
  return rt->addObject(stream, false);
}

/************************************************************************
 * @def GetStreamProperty
 * @brief Return Stream properties
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL
nxsGetStreamProperty(nxs_int stream_id, nxs_uint stream_property_id,
                     void *property_value, size_t *property_value_size) {
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "getStreamProperty " << stream_property_id);
  auto rt = getRuntime();
  auto stream = rt->getPtr<cudaStream_t>(stream_id);
  if (!stream) return NXS_InvalidStream;

  switch (stream_property_id) {
    case NP_Keys: {
      nxs_long keys[] = {NP_Value};
      return rt::getPropertyVec(property_value, property_value_size, keys, 1);
    }
    case NP_Value: {
      return rt::getPropertyInt(property_value, property_value_size,
                                (nxs_long)stream);
    }
  }
  return NXS_Success;
}

/************************************************************************
 * @def ReleaseStream
 * @brief Release the stream on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsReleaseStream(nxs_int stream_id) {
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "releaseStream " << stream_id);
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
                                                  nxs_uint sched_settings) {
  auto rt = getRuntime();
  auto dev = rt->getDevice(device_id);
  if (!dev) return NXS_InvalidDevice;

  auto schedule = rt->getSchedule(device_id, sched_settings);
  if (!schedule) return NXS_InvalidSchedule;
  return rt->addObject(schedule);
}

/************************************************************************
 * @def GetScheduleProperty
 * @brief Return Schedule properties
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL
nxsGetScheduleProperty(nxs_int schedule_id, nxs_uint schedule_property_id,
                       void *property_value, size_t *property_value_size) {
  NXSAPI_LOG(NXSAPI_STATUS_NOTE,
             "getScheduleProperty " << schedule_property_id);
  auto rt = getRuntime();
  auto schedule = rt->get<CudaSchedule>(schedule_id);
  if (!schedule) return NXS_InvalidSchedule;

  switch (schedule_property_id) {
    case NP_Keys: {
      nxs_long keys[] = {NP_ElapsedTime};
      return rt::getPropertyVec(property_value, property_value_size, keys, 1);
    }
    case NP_ElapsedTime: {
      return rt::getPropertyFlt(property_value, property_value_size,
                                schedule->getTime());
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
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "releaseSchedule " << schedule_id);
  auto rt = getRuntime();
  auto sched = rt->get<CudaSchedule>(schedule_id);
  if (!sched) return NXS_InvalidSchedule;
  sched->release();
  if (!rt->dropObject(schedule_id)) return NXS_InvalidSchedule;
  return NXS_Success;
}

/************************************************************************
 * @def RunSchedule
 * @brief Run the schedule on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsRunSchedule(nxs_int schedule_id,
                                                  nxs_int stream_id,
                                                  nxs_uint run_settings) {
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "runSchedule " << schedule_id << " - "
                                                << stream_id << " - "
                                                << run_settings);

  auto rt = getRuntime();

  auto schedule = rt->get<CudaSchedule>(schedule_id);
  if (!schedule) return NXS_InvalidSchedule;

  auto stream = rt->getPtr<cudaStream_t>(stream_id);
  auto status = schedule->run(stream, run_settings);
  if (!nxs_success(status)) return status;

  if (!(run_settings & NXS_ExecutionSettings_NonBlocking)) {
    if (stream)
      CUDA_CHECK(NXS_InvalidStream, cudaStreamSynchronize, stream);
    else
      CUDA_CHECK(NXS_InvalidStream, cudaDeviceSynchronize);
  }
  return NXS_Success;
}

/************************************************************************
 * @def CreateCommand
 * @brief Create command buffer on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
extern "C" nxs_int NXS_API_CALL nxsCreateCommand(nxs_int schedule_id,
                                                 nxs_int kernel_id,
                                                 nxs_uint command_settings) {
  auto rt = getRuntime();

  auto schedule = rt->get<CudaSchedule>(schedule_id);
  if (!schedule) return NXS_InvalidSchedule;

  auto kernel = rt->getPtr<CUfunction>(kernel_id);
  if (!kernel) return NXS_InvalidKernel;

  auto command = rt->getCommand(kernel, command_settings);
  schedule->addCommand(command);
  return rt->addObject(command);
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
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createSignalCommand " << schedule_id << " - "
                                                        << event_id << " - "
                                                        << signal_value);
  auto rt = getRuntime();
  auto sched = rt->get<CudaSchedule>(schedule_id);
  if (!sched) return NXS_InvalidSchedule;
  auto event = rt->getPtr<cudaEvent_t>(event_id);
  if (!event) {
    CUDA_CHECK(NXS_InvalidEvent, cudaEventCreateWithFlags, &event,
               cudaEventDefault);
    rt->addObject(event);
  }

  auto *cmd = rt->getCommand(event, NXS_CommandType_Signal, signal_value,
                             command_settings);
  sched->addCommand(cmd);
  return rt->addObject(cmd);
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
  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createWaitCommand " << schedule_id << " - "
                                                      << event_id << " - "
                                                      << wait_value);
  auto rt = getRuntime();
  auto sched = rt->get<CudaSchedule>(schedule_id);
  if (!sched) return NXS_InvalidSchedule;
  auto event = rt->getPtr<cudaEvent_t>(event_id);
  if (!event) return NXS_InvalidEvent;

  //NXSAPI_LOG(NXSAPI_STATUS_NOTE, "EventQuery: " << hipEventQuery(event));
  auto *cmd =
      rt->getCommand(event, NXS_CommandType_Wait, wait_value, command_settings);
  sched->addCommand(cmd);
  return rt->addObject(cmd);
}

/************************************************************************
 * @def GetCommandProperty
 * @brief Return Command properties
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL
nxsGetCommandProperty(nxs_int command_id, nxs_uint command_property_id,
                      void *property_value, size_t *property_value_size) {
  auto rt = getRuntime();
  auto command = rt->get<CudaCommand>(command_id);
  if (!command) return NXS_InvalidCommand;

  switch (command_property_id) {
    case NP_Keys: {
      nxs_long keys[] = {NP_ElapsedTime};
      return rt::getPropertyVec(property_value, property_value_size, keys, 1);
    }
    case NP_ElapsedTime: {
      return rt::getPropertyFlt(property_value, property_value_size,
                                command->getTime());
    }
  }
  return NXS_Success;
}
/************************************************************************
 * @def SetCommandArgument
 * @brief Set command argument on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsSetCommandArgument(nxs_int command_id,
                                                         nxs_int argument_index,
                                                         nxs_int buffer_id) {
  auto rt = getRuntime();

  auto command = rt->get<CudaCommand>(command_id);
  if (!command) return NXS_InvalidCommand;

  auto buffer = rt->get<rt::Buffer>(buffer_id);
  if (!buffer) return NXS_InvalidBuffer;

  return command->setArgument(argument_index, buffer);
}

/************************************************************************
 * @def SetCommandScalar
 * @brief Set command scalar on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsSetCommandScalar(nxs_int command_id,
                                                       nxs_int argument_index,
                                                       void *value) {
  auto rt = getRuntime();
  auto command = rt->get<CudaCommand>(command_id);
  if (!command) return NXS_InvalidCommand;
  return command->setScalar(argument_index, value);
}
/************************************************************************
 * @def FinalizeCommand
 * @brief Finalize command buffer on the device
 * @return Error status or Succes.
 ***********************************************************************/

extern "C" nxs_status NXS_API_CALL nxsFinalizeCommand(nxs_int command_id,
                                                      nxs_dim3 grid_size,
                                                      nxs_dim3 group_size,
                                                      nxs_uint shared_memory_size) {

  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "finalizeCommand " << command_id << " - "
    << "{ " << grid_size.x <<", " << grid_size.y << ", " << grid_size.z << " }" << " - "
    << "{ " << group_size.x <<", " << group_size.y << ", " << group_size.z << " } " << " - "
    << shared_memory_size);

  auto rt = getRuntime();

  auto command = rt->get<CudaCommand>(command_id);
  if (!command) return NXS_InvalidCommand;

  return command->finalize(grid_size, group_size, shared_memory_size);
}
