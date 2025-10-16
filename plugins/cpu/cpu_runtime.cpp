#include "cpu_runtime.h"

#include <assert.h>
#include <dlfcn.h>
#include <nexus-api.h>
#include <rt_buffer.h>
#include <rt_object.h>
#include <rt_runtime.h>
#include <rt_utilities.h>
#include <string.h>

#include <functional>
#include <magic_enum/magic_enum.hpp>
#include <optional>
#include <vector>

#define NXSAPI_LOG_MODULE "cpu_runtime"

using namespace nxs;

CpuRuntime *getRuntime() {
  static CpuRuntime s_runtime;
  return &s_runtime;
}

#undef NXS_API_CALL
#define NXS_API_CALL __attribute__((visibility("default")))

/************************************************************************
 * @def GetRuntimeProperty
 * @brief Return Runtime properties
 * @return Error status or Succes.
 ************************************************************************/
extern "C" nxs_status NXS_API_CALL
nxsGetRuntimeProperty(nxs_uint runtime_property_id, void *property_value,
                      size_t *property_value_size) {
  auto rt = getRuntime();
  auto proc = cpuinfo_get_processor(0);
  auto *arch = cpuinfo_get_uarch(0);
  auto aid = cpuinfo_get_current_uarch_index();

  NXSAPI_LOG(nexus::NXS_LOG_NOTE, "getRuntimeProperty ", runtime_property_id);

  /* lookup HIP equivalent */
  /* return value size */
  /* return value */
  switch (runtime_property_id) {
    case NP_Keys: {
      nxs_long keys[] = {NP_Name, NP_Type, NP_Vendor, NP_Size, NP_ID};
      int keys_count = sizeof(keys) / sizeof(keys[0]);
      return rt::getPropertyVec(property_value, property_value_size, keys,
                                keys_count);
    }
    case NP_Name:
      return rt::getPropertyStr(property_value, property_value_size, "cpu");
    case NP_Size:
      return rt::getPropertyInt(property_value, property_value_size, 1);
    case NP_Vendor: {
      auto name = cpuinfo_vendor_to_string(proc->core->vendor);
      assert(name);
      return rt::getPropertyStr(property_value, property_value_size,
                                name);
    }
    case NP_Type:
      return rt::getPropertyStr(property_value, property_value_size, "cpu");
    case NP_ID: {
      return rt::getPropertyInt(property_value, property_value_size,
                                cpuinfo_has_arm_sme2() ? 1 : 0);
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
  auto dev = getRuntime()->getObject(device_id);
  if (!dev) return NXS_InvalidDevice;
  auto *cpu = cpuinfo_get_processor(device_id);
  auto *arch = cpuinfo_get_uarch(device_id);
  // auto isa = device->core->isa;

  switch (device_property_id) {
    case NP_Keys: {
      nxs_long keys[] = {NP_Name, NP_Type, NP_Architecture, NP_Size};
      return rt::getPropertyVec(property_value, property_value_size, keys, 4);
    }
    case NP_Name: {
      // return getStr(property_value, property_value_size, device->core);
    }
    case NP_Type:
      return rt::getPropertyStr(property_value, property_value_size, "cpu");
    case NP_Architecture: {
      auto archName = cpuinfo_uarch_to_string(cpu->core->uarch);
      assert(archName);
      return rt::getPropertyStr(property_value, property_value_size,
                                archName);
    }
    case NP_Size:
      return rt::getPropertyInt(property_value, property_value_size,
                                cpuinfo_get_processors_count());

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
  auto dev = rt->getObject(device_id);
  if (!dev) return NXS_InvalidDevice;

  NXSAPI_LOG(nexus::NXS_LOG_NOTE, "createBuffer ", size);
  auto *buf = rt->getBuffer(size, host_ptr, false);
  if (!buf) return NXS_InvalidBuffer;

  return rt->addObject(buf);
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
  auto buf = rt->getObject(buffer_id);
  if (!buf) return NXS_InvalidBuffer;
  auto bufObj = (*buf)->get<rt::Buffer>();
  std::memcpy(host_ptr, bufObj->data(), bufObj->size());
  return NXS_Success;
}

/************************************************************************
 * @def ReleaseBuffer
 * @brief Release a buffer on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsReleaseBuffer(nxs_int buffer_id) {
  auto rt = getRuntime();
  NXSAPI_LOG(nexus::NXS_LOG_NOTE, "releaseBuffer ", buffer_id);
  return rt->releaseBuffer(buffer_id);
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
  auto dev = rt->getObject(device_id);
  if (!dev) return NXS_InvalidDevice;

  // #include <sys/mman.h>
  // #include <dlfcn.h>
  // #include <unistd.h>
  // int fd = memfd_create("my_lib", MFD_CLOEXEC);
  // write(fd, library_data, library_size);
  // char fd_path[64];
  // snprintf(fd_path, sizeof(fd_path), "/proc/self/fd/%d", fd);
  // void *handle = dlopen(fd_path, RTLD_NOW);
  // return rt->addObject(lib);
  return NXS_InvalidLibrary;
}

/************************************************************************
 * @def CreateLibraryFromFile
 * @brief Create a library from a file
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_int NXS_API_CALL nxsCreateLibraryFromFile(
    nxs_int device_id, const char *library_path, nxs_uint settings) {
  NXSAPI_LOG(nexus::NXS_LOG_NOTE,
             "createLibraryFromFile ", device_id, " - ", library_path);
  auto rt = getRuntime();
  auto dev = rt->getObject(device_id);
  if (!dev) return NXS_InvalidDevice;

  void *lib = dlopen(library_path, RTLD_NOW);
  if (!lib) {
    NXSAPI_LOG(nexus::NXS_LOG_ERROR, "createLibraryFromFile ", dlerror());
    return NXS_InvalidLibrary;
  }
  return rt->addObject(lib);
}

/************************************************************************
 * @def GetLibraryProperty
 * @brief Return Library properties
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL
nxsGetLibraryProperty(nxs_int library_id, nxs_uint library_property_id,
                      void *property_value, size_t *property_value_size) {
  return NXS_InvalidProperty;
}

/************************************************************************
 * @def ReleaseLibrary
 * @brief Release a library on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsReleaseLibrary(nxs_int library_id) {
  auto rt = getRuntime();
  auto lib = rt->getObject(library_id);
  if (!lib) return NXS_InvalidLibrary;
  dlclose((*lib)->get<void>());
  rt->dropObject(library_id);
  return NXS_Success;
}

/************************************************************************
 * @def GetKernel
 * @brief Lookup a kernel in a library
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_int NXS_API_CALL nxsGetKernel(nxs_int library_id,
                                             const char *kernel_name) {
  NXSAPI_LOG(nexus::NXS_LOG_NOTE,
             "getKernel ", library_id, " - ", kernel_name);
  auto rt = getRuntime();
  auto lib = rt->getObject(library_id);
  if (!lib) return NXS_InvalidProgram;
  void *func = dlsym((*lib)->get<void>(), kernel_name);
  if (!func) {
    NXSAPI_LOG(nexus::NXS_LOG_ERROR, "getKernel ", dlerror());
    return NXS_InvalidKernel;
  }
  return rt->addObject(func);
}

/************************************************************************
 * @def GetKernelProperty
 * @brief Return Kernel properties
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL
nxsGetKernelProperty(nxs_int kernel_id, nxs_uint kernel_property_id,
                     void *property_value, size_t *property_value_size) {
  auto rt = getRuntime();
  auto func = rt->getObject(kernel_id);
  if (!func) return NXS_InvalidKernel;

  switch (kernel_property_id) {
    default:
      return NXS_InvalidProperty;
  }

  return NXS_Success;
}

/************************************************************************
 * @def ReleaseKernel
 * @brief Release a kernel on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsReleaseKernel(nxs_int kernel_id) {
  auto rt = getRuntime();
  if (!rt->dropObject(kernel_id)) return NXS_InvalidKernel;
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
  auto dev = rt->getObject(device_id);
  if (!dev) return NXS_InvalidDevice;

  // 1 CPU, with many cores and 1 thread per core
  return NXS_Success;
}

/************************************************************************
 * @def ReleaseStream
 * @brief Release the stream on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsReleaseStream(nxs_int stream_id) {
  // auto rt = getRuntime();
  // if (!rt->dropObject<MTL::CommandQueue>(stream_id))
  //   return NXS_InvalidStream;
  return NXS_Success;
}

/************************************************************************
 * @def CreateSchedule
 * @brief Create schedule on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
extern "C" nxs_int NXS_API_CALL nxsCreateSchedule(nxs_int device_id,
                                                  nxs_uint schedule_settings) {
  auto rt = getRuntime();
  auto dev = rt->getObject(device_id);
  if (!dev) return NXS_InvalidDevice;

  return rt->getSchedule(device_id, schedule_settings);
}

/************************************************************************
 * @def GetScheduleProperty
 * @brief Return Schedule properties
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL
nxsGetScheduleProperty(nxs_int schedule_id, nxs_uint schedule_property_id,
                       void *property_value, size_t *property_value_size) {
  NXSAPI_LOG(nexus::NXS_LOG_NOTE,
             "getScheduleProperty ", schedule_property_id);
  auto rt = getRuntime();
  auto schedule = rt->get<CpuSchedule>(schedule_id);
  if (!schedule) return NXS_InvalidSchedule;

  switch (schedule_property_id) {
    case NP_Keys: {
      constexpr nxs_long keys[] = {NP_ElapsedTime};
      constexpr int keys_count = sizeof(keys) / sizeof(keys[0]);
      return rt::getPropertyVec(property_value, property_value_size, keys,
                                keys_count);
    }
    case NP_ElapsedTime: {
      return rt::getPropertyFlt(property_value, property_value_size,
                                schedule->getTime());
    }
  }
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
  auto rt = getRuntime();
  auto schedule = rt->get<CpuSchedule>(schedule_id);
  if (!schedule) return NXS_InvalidSchedule;

  return schedule->run(stream_id, run_settings);
}

/************************************************************************
 * @def ReleaseSchedule
 * @brief Release the schedule on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsReleaseSchedule(nxs_int schedule_id) {
  auto rt = getRuntime();
  return rt->releaseSchedule(schedule_id);
}

/************************************************************************
 * @def CreateCommand
 * @brief Create command on the device
 * @return Negative value is an error status.
 *         Non-negative is the bufferId.
 ***********************************************************************/
extern "C" nxs_int NXS_API_CALL nxsCreateCommand(nxs_int schedule_id,
                                                 nxs_int kernel_id,
                                                 nxs_uint settings) {
  auto rt = getRuntime();
  auto schedule = rt->get<CpuSchedule>(schedule_id);
  if (!schedule) return NXS_InvalidSchedule;
  auto kernel_v = rt->get<void>(kernel_id);
  if (!kernel_v) return NXS_InvalidKernel;
  auto kernel = reinterpret_cast<cpuFunction_t>(kernel_v);

  auto command = rt->getCommand(kernel, settings);
  schedule->addCommand(command);
  return rt->addObject(command);
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

  auto command = rt->get<CpuCommand>(command_id);
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
  auto command = rt->get<CpuCommand>(command_id);
  if (!command) return NXS_InvalidCommand;
  return command->setScalar(argument_index, value);
}

/************************************************************************
 * @def FinalizeCommand
 * @brief Finalize command setup
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsFinalizeCommand(nxs_int command_id,
                                                      nxs_dim3 grid_size,
                                                      nxs_dim3 group_size,
                                                      nxs_uint shared_memory_size) {

  NXSAPI_LOG(nexus::NXS_LOG_NOTE, "finalizeCommand ", command_id, " - "
  , "{ ", grid_size.x,", ", grid_size.y,", ", grid_size.z, " }", " - "
  , "{ ", group_size.x,", ", group_size.y,", ", group_size.z, " }");

  auto rt = getRuntime();

  auto command = rt->get<CpuCommand>(command_id);
  if (!command) return NXS_InvalidCommand;

  return command->finalize(grid_size, group_size, shared_memory_size);
}
