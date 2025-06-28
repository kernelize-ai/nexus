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
RuntimeImpl::RuntimeImpl(Impl owner, const std::string &path)
    : Impl(owner), pluginLibraryPath(path), library(nullptr) {
  NEXUS_LOG(NEXUS_STATUS_NOTE, "  CTOR: " << path);
  loadPlugin();
}

RuntimeImpl::~RuntimeImpl() {
  NEXUS_LOG(NEXUS_STATUS_NOTE, "  DTOR: " << pluginLibraryPath);
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
  NEXUS_LOG(NEXUS_STATUS_NOTE, "Runtime.getProperty: " << nxsGetPropName(prop));
  if (auto fn = getFunction<NF_nxsGetRuntimeProperty>()) {
    auto npt_prop = nxs_property_type_map[prop];
    if (npt_prop == NPT_INT) {
      nxs_long val = 0;
      size_t size = sizeof(val);
      if (nxs_success((*fn)(prop, &val, &size))) return Property(val);
    } else if (npt_prop == NPT_FLT) {
      nxs_double val = 0.;
      size_t size = sizeof(val);
      if (nxs_success((*fn)(prop, &val, &size))) return Property(val);
    } else if (npt_prop == NPT_STR) {
      size_t size = 256;
      char name[size];
      name[0] = '\0';
      if (nxs_success((*fn)(prop, &name, &size))) return std::string(name);
    } else {
      NEXUS_LOG(NEXUS_STATUS_ERR,
                "Runtime.getProperty: Unknown property type for - "
                    << nxsGetPropName(prop));
      // assert(0);
    }
  }
  return std::nullopt;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void RuntimeImpl::loadPlugin() {
  NEXUS_LOG(NEXUS_STATUS_NOTE, "Loading Runtime plugin: " << pluginLibraryPath);
  library = dlopen(pluginLibraryPath.c_str(), RTLD_NOW | RTLD_GLOBAL);
  char *dlError = dlerror();
  if (dlError) {
    NEXUS_LOG(NEXUS_STATUS_ERR, "  Failed to dlopen plugin: " << dlError);
    assert(0);
  } else if (library == nullptr) {
    NEXUS_LOG(NEXUS_STATUS_ERR, "  Failed to load plugin");
    assert(0);
  }

  auto loadFn = [&](nxs_function fn) {
    auto *fName = nxsGetFuncName(fn);
    runtimeFns[fn] = dlsym(library, fName);
    dlError = dlerror();
    if (dlError) {
      NEXUS_LOG(NEXUS_STATUS_WARN,
                "  Failed to load symbol '" << fName << "': " << dlError);
    } else {
      NEXUS_LOG(
          NEXUS_STATUS_NOTE,
          "  Loaded symbol: " << fName << " - " << (int64_t)runtimeFns[fn]);
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
Runtime::Runtime(detail::Impl owner, const std::string &libraryPath)
    : Object(owner, libraryPath) {}

nxs_int Runtime::getId() const { NEXUS_OBJ_MCALL(NXS_InvalidRuntime, getId); }

Devices Runtime::getDevices() const { NEXUS_OBJ_MCALL(Devices(), getDevices); }

Device Runtime::getDevice(nxs_uint deviceId) const {
  NEXUS_OBJ_MCALL(Device(), getDevice, deviceId);
}

// Get Runtime Property Value
std::optional<Property> Runtime::getProperty(nxs_int prop) const {
  NEXUS_OBJ_MCALL(std::nullopt, getProperty, prop);
}
