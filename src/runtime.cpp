#include <assert.h>
#include <dlfcn.h>
#include <nexus/device_db.h>
#include <nexus/log.h>
#include <nexus/runtime.h>

#include "_runtime_impl.h"

using namespace nexus;
using namespace nexus::detail;

#define NEXUS_LOG_MODULE "runtime"

/// @brief Construct a Runtime for the current system
RuntimeImpl::RuntimeImpl(Impl base, const std::string &path)
    : Impl(base), pluginLibraryPath(path), library(nullptr) {
  NEXUS_LOG(NXS_LOG_NOTE, "  CTOR: ", path);
  loadPlugin();
}

RuntimeImpl::~RuntimeImpl() {
  NEXUS_LOG(NXS_LOG_NOTE, "  DTOR: ", pluginLibraryPath);
  release();
  if (library != nullptr) dlclose(library);
}

void RuntimeImpl::release() {
  devices.clear();
}

Device RuntimeImpl::getDevice(nxs_int deviceId) const {
  if (deviceId < 0 || deviceId >= devices.size()) return Device();
  return devices.get(deviceId);
}

std::optional<Property> detail::RuntimeImpl::getProperty(nxs_int prop) const {
  return getAPIProperty<NF_nxsGetRuntimeProperty>(prop);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void RuntimeImpl::loadPlugin() {
  NEXUS_LOG(NXS_LOG_NOTE, "Loading Runtime plugin: ", pluginLibraryPath);
  library = dlopen(pluginLibraryPath.c_str(), RTLD_NOW | RTLD_GLOBAL);
  char *dlError = dlerror();
  if (dlError || library == nullptr) {
    if (dlError)
      NEXUS_LOG(NXS_LOG_ERROR, "  Failed to dlopen plugin: ", dlError);
    else
      NEXUS_LOG(NXS_LOG_ERROR, "  Failed to load plugin: ", pluginLibraryPath);
    memset(runtimeFns, 0, sizeof(runtimeFns));
    library = nullptr;
    return;
  }

  auto loadFn = [&](nxs_function fn) {
    auto *fName = nxsGetFuncName(fn);
    runtimeFns[fn] = dlsym(library, fName);
    dlError = dlerror();
    if (dlError) {
      NEXUS_LOG(NXS_LOG_WARN,
                "  Failed to load symbol '", fName, "': ", dlError);
    } else {
      NEXUS_LOG(
          NXS_LOG_NOTE,
          "  Loaded symbol: ", fName, " - ", (int64_t)runtimeFns[fn]);
    }
  };

  for (int fn = 0; fn < NXS_FUNCTION_CNT; ++fn) {
    loadFn((nxs_function)fn);
  }

  if (!runtimeFns[NF_nxsGetRuntimeProperty] ||
      !runtimeFns[NF_nxsGetDeviceProperty])
    return;

  // Load devices
  if (auto deviceCount = getProperty(NP_Size)) {
    for (int i = 0; i < deviceCount->getValue<nxs_long>(); ++i)
      devices.add(Impl(this, i)); // DEVICE IDs MUST BE 0..N
  }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
Runtime::Runtime(detail::Impl base, const std::string &libraryPath)
    : Object(base, libraryPath) {}

Devices Runtime::getDevices() const { NEXUS_OBJ_MCALL(Devices(), getDevices); }

Device Runtime::getDevice(nxs_uint deviceId) const {
  NEXUS_OBJ_MCALL(Device(), getDevice, deviceId);
}

// Get Runtime Property Value
std::optional<Property> Runtime::getProperty(nxs_int prop) const {
  NEXUS_OBJ_MCALL(std::nullopt, getProperty, prop);
}
