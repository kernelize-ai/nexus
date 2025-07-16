#include <assert.h>
#include <cpuinfo.h>
#include <dlfcn.h>
#include <rt_buffer.h>
#include <rt_object.h>
#include <rt_runtime.h>
#include <rt_utilities.h>
#include <string.h>

#include <optional>
#include <functional>
#include <optional>
#include <vector>

#define NXSAPI_LOGGING
#include <nexus-api.h>

#define NXSAPI_LOG_MODULE "cpu"

using namespace nxs;

class CpuRuntime : public rt::Runtime {
 public:
  CpuRuntime() : rt::Runtime() {
    cpuinfo_initialize();
    for (size_t i = 0; i < cpuinfo_get_processors_count(); i++) {
      auto *cpu = cpuinfo_get_processor(i);
      addObject(nullptr, (void *)cpu);
    }
  }
  ~CpuRuntime() {}
};

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

  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "getRuntimeProperty " << runtime_property_id);

  /* lookup HIP equivalent */
  /* return value size */
  /* return value */
  switch (runtime_property_id) {
    case NP_Name:
      return rt::getPropertyStr(property_value, property_value_size,
                                "cpu-generic");
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
    case NP_Architecture: {
      auto name = cpuinfo_uarch_to_string(arch->uarch);
      assert(name);
      return rt::getPropertyStr(property_value, property_value_size,
                                name);
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
  auto device = (*dev)->get<cpuinfo_processor>();
  // auto isa = device->core->isa;

  switch (device_property_id) {
    case NP_Name: {
      // return getStr(property_value, property_value_size, device->core);
    }
    case NP_Type:
      return rt::getPropertyStr(property_value, property_value_size, "cpu");
    case NP_Architecture: {
      auto archName = cpuinfo_uarch_to_string(device->core->uarch);
      assert(archName);
      return rt::getPropertyStr(property_value, property_value_size,
                                archName);
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
  auto dev = rt->getObject(device_id);
  if (!dev) return NXS_InvalidDevice;

  NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createBuffer " << size);
  rt::Buffer *buf = new rt::Buffer(size, host_ptr, true);
  return rt->addObject(*dev, buf, true);
}

/************************************************************************
 * @def CopyBuffer
 * @brief Copy a buffer to the host
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsCopyBuffer(nxs_int buffer_id,
                                                 void *host_ptr) {
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
  if (!rt->dropObject(buffer_id, rt::delete_fn<rt::Buffer>))
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
  auto dev = rt->getObject(device_id);
  if (!dev) return NXS_InvalidDevice;

  // NS::Array *binArr = NS::Array::alloc();
  // MTL::StitchedLibraryDescriptor *libDesc =
  // MTL::StitchedLibraryDescriptor::alloc(); libDesc->init(); // IS THIS
  // NECESSARY? libDesc->setBinaryArchives(binArr);
  // dispatch_data_t data = (dispatch_data_t)library_data;
  // NS::Error *pError = nullptr;
  // MTL::Library *pLibrary = device->newLibrary(data, &pError);
  // MTL::Library *pLibrary = (*dev)->newLibrary(
  // NS::String::string("kernel.so", NS::UTF8StringEncoding), &pError);
  // NXSAPI_LOG(NXSAPI_STATUS_NOTE,
  //            "createLibrary " << (int64_t)pError << " - " <<
  //            (int64_t)pLibrary);
  //
  // if (pError) {
  //   NXSAPI_LOG(
  //       NXSAPI_STATUS_ERR,
  //       "createLibrary " << pError->localizedDescription()->utf8String());
  //   return NXS_InvalidLibrary;
  // }
  // return rt->addObject(pLibrary);
  return NXS_Success;
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
  auto dev = rt->getObject(device_id);
  if (!dev) return NXS_InvalidDevice;

  void *lib = dlopen(library_path, RTLD_NOW);
  if (!lib) {
    NXSAPI_LOG(NXSAPI_STATUS_ERR, "createLibraryFromFile " << dlerror());
    return NXS_InvalidLibrary;
  }
  return rt->addObject(*dev, lib);
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
  NXSAPI_LOG(NXSAPI_STATUS_NOTE,
             "getKernel " << library_id << " - " << kernel_name);
  auto rt = getRuntime();
  auto lib = rt->getObject(library_id);
  if (!lib) return NXS_InvalidProgram;
  void *func = dlsym((*lib)->get<void>(), kernel_name);
  if (!func) {
    NXSAPI_LOG(NXSAPI_STATUS_ERR, "getKernel " << dlerror());
    return NXS_InvalidKernel;
  }
  return rt->addObject(*lib, func);
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
                                                nxs_uint stream_properties) {
  auto rt = getRuntime();
  auto dev = rt->getObject(device_id);
  if (!dev) return NXS_InvalidDevice;

  // spin up a thread.. see processor affinity
  // NXSAPI_LOG(NXSAPI_STATUS_NOTE, "createStream");
  // MTL::CommandQueue *stream = (*dev)->newCommandQueue();
  // return rt->addObject(stream);
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
                                                  nxs_uint sched_properties) {
  auto rt = getRuntime();
  auto dev = rt->getObject(device_id);
  if (!dev) return NXS_InvalidDevice;

  return rt->addObject(*dev);
}

/************************************************************************
 * @def ReleaseCommandList
 * @brief Release the buffer on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsRunSchedule(nxs_int schedule_id,
                                                  nxs_int stream_id,
                                                  nxs_bool blocking) {
  auto rt = getRuntime();
  auto sched = rt->getObject(schedule_id);
  if (!sched) return NXS_InvalidDevice;

  for (auto cmdId : (*sched)->getChildren()) {
    auto cmd = rt->getObject(cmdId);
    if (!cmd) return NXS_InvalidCommand;
    auto kernel = rt->get<rt::Object>(cmdId);
    if (!kernel) return NXS_InvalidKernel;
    auto func = (*kernel)->get<void>();
    if (!func) return NXS_InvalidKernel;
    auto func_ptr = (void (*)(void *, void *, void *, void *, void *, void *,
                              void *, void *, void *, void *, void *, void *,
                              void *, void *, void *, void *, void *, void *,
                              void *, void *, void *, void *, void *, void *,
                              void *, void *, void *, void *, void *, void *,
                              void *, void *))func;

    auto &args = (*cmd)->getChildren();

    if (args.size() >= 32) {
      NXSAPI_LOG(NXSAPI_STATUS_ERR, "Too many arguments for kernel");
      return NXS_InvalidCommand;
    }
    std::vector<char> exData(1024 * 1024);  // 1MB extra buffer for args
    rt::Buffer exBuf(exData.size(), exData.data(),
                     false);                     // extra buffer for args
    std::vector<rt::Buffer *> bufs(32, &exBuf);  // max 32 args
    for (size_t i = 0; i < args.size(); i++) {
      auto buf = rt->getObject(args[i]);
      if (!buf) return NXS_InvalidBuffer;
      bufs[i] = (*buf)->get<rt::Buffer>();
    }
    std::vector<int64_t> coords{0, 0, 0};
    rt::Buffer coordsBuf(sizeof(coords), coords.data());
    bufs[args.size()] = &coordsBuf;

    // call func with bufs + dims (int64[3], int64[3], int64[3])
    int64_t *global_size = bufs[args.size() - 2]->get<int64_t>();
    int64_t *local_size = bufs[args.size() - 1]->get<int64_t>();
    for (int64_t i = 0; i < global_size[0]; i += local_size[0]) {
      for (int64_t j = 0; j < global_size[1]; j += local_size[1]) {
        for (int64_t k = 0; k < global_size[2]; k += local_size[2]) {
          coords[0] = i;
          coords[1] = j;
          coords[2] = k;
          try {
            std::invoke(func_ptr, bufs[0]->data(), bufs[1]->data(),
                        bufs[2]->data(), bufs[3]->data(), bufs[4]->data(),
                        bufs[5]->data(), bufs[6]->data(), bufs[7]->data(),
                        bufs[8]->data(), bufs[9]->data(), bufs[10]->data(),
                        bufs[11]->data(), bufs[12]->data(), bufs[13]->data(),
                        bufs[14]->data(), bufs[15]->data(), bufs[16]->data(),
                        bufs[17]->data(), bufs[18]->data(), bufs[19]->data(),
                        bufs[20]->data(), bufs[21]->data(), bufs[22]->data(),
                        bufs[23]->data(), bufs[24]->data(), bufs[25]->data(),
                        bufs[26]->data(), bufs[27]->data(), bufs[28]->data(),
                        bufs[29]->data(), bufs[30]->data(), bufs[31]->data());
          } catch (const std::exception &e) {
            NXSAPI_LOG(NXSAPI_STATUS_ERR, "runSchedule: " << e.what());
          }
        }
      }
    }
  }

  if (blocking) {
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
  if (!rt->dropObject(schedule_id)) return NXS_InvalidBuildOptions;  // fix
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
  auto sched = rt->getObject(schedule_id);
  if (!sched) return NXS_InvalidBuildOptions;  // fix
  auto kernel = rt->getObject(kernel_id);
  if (!kernel) return NXS_InvalidKernel;

  auto cmdId = rt->addObject(*sched, *kernel, false);
  if (nxs_success(cmdId)) (*sched)->addChild(cmdId);
  return cmdId;
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
  auto cmd = rt->getObject(command_id);
  if (!cmd) return NXS_InvalidCommand;
  auto buf = rt->getObject(buffer_id);
  if (!buf) return NXS_InvalidBuffer;

  (*cmd)->addChild(buffer_id, argument_index);
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
  auto cmd = rt->getObject(command_id);
  if (!cmd) return NXS_InvalidCommand;

  int64_t global_size[3] = {grid_size, 1, 1};
  auto global_buf = new rt::Buffer(sizeof(global_size), global_size, true);
  int64_t local_size[3] = {group_size, 1, 1};
  auto local_buf = new rt::Buffer(sizeof(local_size), local_size, true);
  (*cmd)->addChild(rt->addObject(*cmd, global_buf, true));
  (*cmd)->addChild(rt->addObject(*cmd, local_buf, true));
  return NXS_Success;
}

/************************************************************************
 * @def ReleaseCommand
 * @brief Release the command on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsReleaseCommand(nxs_int command_id) {
  auto rt = getRuntime();
  auto cmd = rt->getObject(command_id);
  if (!cmd) return NXS_InvalidCommand;
  auto children = (*cmd)->getChildren();
  if (children.size() < 2) return NXS_InvalidCommand;
  auto global_size_id = children[children.size() - 2];
  auto local_size_id = children[children.size() - 1];
  if (!rt->dropObject(global_size_id, rt::delete_fn<rt::Buffer>))
    return NXS_InvalidBuffer;
  if (!rt->dropObject(local_size_id, rt::delete_fn<rt::Buffer>))
    return NXS_InvalidBuffer;
  if (!rt->dropObject(command_id)) return NXS_InvalidCommand;
  return NXS_Success;
}
