#include <assert.h>
#include <cpuinfo.h>
#include <dlfcn.h>
#include <rt_utilities.h>
#include <string.h>

#include <iostream>
#include <magic_enum/magic_enum.hpp>
#include <optional>
#include <vector>

#define NXSAPI_LOGGING
#include <nexus-api.h>

#define NXSAPI_LOG_MODULE "cpu"

class RTBuffer {
  char *buf;
  size_t sz;
  bool is_owned;

 public:
  RTBuffer(size_t size, void *host_ptr = nullptr, bool is_owned = false)
      : buf((char *)host_ptr), sz(size), is_owned(is_owned) {
    if (host_ptr && is_owned) {
      buf = (char *)malloc(size);
      memcpy(buf, host_ptr, size);
    }
  }
  ~RTBuffer() {
    if (is_owned && buf) free(buf);
  }
  char *data() { return buf; }
  size_t size() { return sz; }
  template <typename T>
  T *get() {
    return (T *)buf;
  }
};

class RTObject {
  void *obj;
  bool is_owned;
  std::vector<nxs_int> children;

 public:
  RTObject(void *obj, bool is_owned = true) {
    this->obj = obj;
    this->is_owned = obj ? is_owned : false;
  }
  virtual ~RTObject() {}
  operator bool() const { return obj != nullptr || is_owned; }

  template <typename T = void>
  T *get() const {
    return static_cast<T *>(obj);
  }

  template <typename T = void>
  void release() {
    children.clear();
    if constexpr (!std::is_same_v<T, void>) {
      if (is_owned && obj) delete static_cast<T *>(obj);
    }
    obj = nullptr;
    is_owned = false;
  }
  std::vector<nxs_int> &getChildren() { return children; }
  void addChild(nxs_int child, nxs_int index = -1) {
    if (index < 0)
      children.push_back(child);
    else {
      if (index >= children.size()) children.resize(index + 1);
      children[index] = child;
    }
  }
};

class CpuRuntime {
  std::vector<RTObject> objects;

 public:
  CpuRuntime() {
    objects.reserve(1024);
    cpuinfo_initialize();
    for (size_t i = 0; i < cpuinfo_get_processors_count(); i++) {
      auto *cpu = cpuinfo_get_processor(i);
      objects.emplace_back((void *)cpu, false);
    }
  }
  ~CpuRuntime() {
    // for (auto &obj : objects)
    //   obj.release();
  }

  nxs_int addObject(void *obj, bool is_owned = true) {
    objects.emplace_back(obj, is_owned);
    return objects.size() - 1;
  }

  std::optional<RTObject *> getObject(nxs_int id) {
    if (id < 0 || id >= objects.size()) return std::nullopt;
    if (objects[id]) return &objects[id];
    return std::nullopt;
  }
  template <typename T>
  bool dropObject(nxs_int id) {
    if (id < 0 || id >= objects.size()) return false;
    auto &obj = objects[id];
    if (obj) {
      obj.release<T>();
      return true;
    }
    return false;
  }
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
      return rt_getPropertyStr(property_value, property_value_size,
                               "cpu-generic");
    case NP_Size:
      return rt_getPropertyInt(property_value, property_value_size, 1);
    case NP_Vendor: {
      auto name = magic_enum::enum_name(proc->core->vendor);
      return rt_getPropertyStr(property_value, property_value_size,
                               name.data());
    }
    case NP_Type:
      return rt_getPropertyStr(property_value, property_value_size, "cpu");
    case NP_ID: {
      return rt_getPropertyInt(property_value, property_value_size,
                               cpuinfo_has_arm_sme2() ? 1 : 0);
    }
    case NP_Architecture: {
      auto name = magic_enum::enum_name(arch->uarch);
      return rt_getPropertyStr(property_value, property_value_size,
                               name.data());
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
      return rt_getPropertyStr(property_value, property_value_size, "cpu");
    case NP_Architecture: {
      auto archName = magic_enum::enum_name(device->core->uarch);
      return rt_getPropertyStr(property_value, property_value_size,
                               archName.data());
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
  RTBuffer *buf = new RTBuffer(size, host_ptr, true);
  return rt->addObject(buf, true);
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
  auto bufObj = (*buf)->get<RTBuffer>();
  memcpy(host_ptr, bufObj->data(), bufObj->size());
  return NXS_Success;
}

/************************************************************************
 * @def ReleaseBuffer
 * @brief Release a buffer on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsReleaseBuffer(nxs_int buffer_id) {
  auto rt = getRuntime();
  if (!rt->dropObject<RTBuffer>(buffer_id)) return NXS_InvalidBuffer;
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
  return rt->addObject(lib, false);
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
  rt->dropObject<void>(library_id);
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
  return rt->addObject(func, false);
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
  if (!rt->dropObject<void>(kernel_id)) return NXS_InvalidKernel;
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

  char *name = new char[10];
  strcpy(name, "schedule");
  return rt->addObject(name, true);
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
    auto func = (*cmd)->get<void>();
    if (!func) return NXS_InvalidCommand;
    auto &args = (*cmd)->getChildren();

    if (args.size() >= 32) {
      NXSAPI_LOG(NXSAPI_STATUS_ERR, "Too many arguments for kernel");
      return NXS_InvalidCommand;
    }
    std::vector<char> exData(1024 * 1024);  // 1MB extra buffer for args
    RTBuffer exBuf(exData.size(), exData.data(),
                   false);                     // extra buffer for args
    std::vector<RTBuffer *> bufs(32, &exBuf);  // max 32 args
    for (size_t i = 0; i < args.size(); i++) {
      auto buf = rt->getObject(args[i]);
      if (!buf) return NXS_InvalidBuffer;
      bufs[i] = (*buf)->get<RTBuffer>();
    }
    std::vector<int64_t> coords{0, 0, 0};
    RTBuffer coordsBuf(sizeof(coords), coords.data());
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
            ((void (*)(void *, void *, void *, void *, void *, void *, void *,
                       void *, void *, void *, void *, void *, void *, void *,
                       void *, void *, void *, void *, void *, void *, void *,
                       void *, void *, void *, void *, void *, void *, void *,
                       void *, void *, void *, void *))func)(
                bufs[0]->data(), bufs[1]->data(), bufs[2]->data(),
                bufs[3]->data(), bufs[4]->data(), bufs[5]->data(),
                bufs[6]->data(), bufs[7]->data(), bufs[8]->data(),
                bufs[9]->data(), bufs[10]->data(), bufs[11]->data(),
                bufs[12]->data(), bufs[13]->data(), bufs[14]->data(),
                bufs[15]->data(), bufs[16]->data(), bufs[17]->data(),
                bufs[18]->data(), bufs[19]->data(), bufs[20]->data(),
                bufs[21]->data(), bufs[22]->data(), bufs[23]->data(),
                bufs[24]->data(), bufs[25]->data(), bufs[26]->data(),
                bufs[27]->data(), bufs[28]->data(), bufs[29]->data(),
                bufs[30]->data(), bufs[31]->data());
          } catch (const std::exception &e) {
            NXSAPI_LOG(NXSAPI_STATUS_ERR, "runSchedule: " << e.what());
          }
        }
      }
    }
  }

  // auto stream = rt->getObject<MTL::CommandQueue>(stream_id);
  // if (stream)
  //   assert((*cmdbuf)->commandQueue() == *stream);

  // (*cmdbuf)->enqueue();

  // (*cmdbuf)->commit();
  if (blocking) {
    // (*cmdbuf)->waitUntilCompleted();  // Synchronous wait for simplicity
    // if ((*cmdbuf)->status() == MTL::CommandBufferStatusError) {
    //   NXSAPI_LOG(
    //       NXSAPI_STATUS_ERR,
    //       "runSchedule: "
    //           << (*cmdbuf)->error()->localizedDescription()->utf8String());
    //   return NXS_InvalidEvent;
    // }
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
  if (!rt->dropObject<char>(schedule_id))
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
  auto sched = rt->getObject(schedule_id);
  if (!sched) return NXS_InvalidBuildOptions;  // fix
  auto kernel = rt->getObject(kernel_id);
  if (!kernel) return NXS_InvalidKernel;
  auto cmdId = rt->addObject((*kernel)->get<void>(), false);
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
  int64_t local_size[3] = {group_size, 1, 1};
  (*cmd)->addChild(rt->addObject(
      new RTBuffer(sizeof(global_size), global_size, true), true));
  (*cmd)->addChild(
      rt->addObject(new RTBuffer(sizeof(local_size), local_size, true), true));
  return NXS_Success;
}

/************************************************************************
 * @def ReleaseCommand
 * @brief Release the command on the device
 * @return Error status or Succes.
 ***********************************************************************/
extern "C" nxs_status NXS_API_CALL nxsReleaseCommand(nxs_int command_id) {
  auto rt = getRuntime();
  if (!rt->dropObject<void>(command_id)) return NXS_InvalidBuildOptions;  // fix
  return NXS_Success;
}
