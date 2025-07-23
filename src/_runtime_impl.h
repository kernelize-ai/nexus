#ifndef _NEXUS_RUNTIME_IMPL_H
#define _NEXUS_RUNTIME_IMPL_H

#include <nexus-api.h>
#include <nexus/device.h>

#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#define NEXUS_LOG_MODULE "runtime"

namespace nexus {

namespace detail {
class RuntimeImpl : public Impl {
 public:
  RuntimeImpl(Impl owner, const std::string &path);
  ~RuntimeImpl();

  void release();

  std::optional<Property> getProperty(nxs_int prop) const;

  Devices getDevices() const { return devices; }
  Device getDevice(nxs_int deviceId) const;

  template <nxs_function Tfn,
            typename Tfnp = typename nxsFunctionType<Tfn>::type>
  Tfnp getFunction() const {
    return (Tfnp)runtimeFns[Tfn];
  }

  template <nxs_function Tfn, typename... Args>
  nxs_int runAPIFunction(Args... args) {
    nxs_int apiResult = NXS_InvalidDevice;  // invalid runtime
    if (auto *fn = getFunction<Tfn>()) {
      apiResult = (*fn)(args...);
      if (nxs_failed(apiResult))
        NEXUS_LOG(NEXUS_STATUS_ERR, nxsGetFuncName(Tfn)
                                        << ": " << nxsGetStatusName(apiResult));
      else
        NEXUS_LOG(NEXUS_STATUS_NOTE, nxsGetFuncName(Tfn) << ": " << apiResult);
    } else {
      NEXUS_LOG(NEXUS_STATUS_ERR, nxsGetFuncName(Tfn) << ": API not present");
    }
    return apiResult;
  }

  template <nxs_function Tfn, typename... Args>
  std::optional<Property> getAPIProperty(nxs_int prop, Args... args) const {
    if (auto fn = getFunction<Tfn>()) {
      auto npt_prop = nxs_property_type_map[prop];
      switch (npt_prop) {
        case NPT_INT: {
          nxs_long val = 0;
          size_t size = sizeof(val);
          if (nxs_success((*fn)(args..., prop, &val, &size))) {
            NEXUS_LOG(NEXUS_STATUS_NOTE, nxsGetFuncName(Tfn)
                                             << ": " << nxsGetPropName(prop)
                                             << " = " << val);
            return Property(val);
          }
          break;
        }
        case NPT_FLT: {
          nxs_double val = 0.;
          size_t size = sizeof(val);
          if (nxs_success((*fn)(args..., prop, &val, &size))) {
            NEXUS_LOG(NEXUS_STATUS_NOTE, nxsGetFuncName(Tfn)
                                             << ": " << nxsGetPropName(prop)
                                             << " = " << val);
            return Property(val);
          }
          break;
        }
        case NPT_STR: {
          size_t size = 256;
          char name[size];
          name[0] = '\0';
          if (nxs_success((*fn)(args..., prop, &name, &size))) {
            NEXUS_LOG(NEXUS_STATUS_NOTE, nxsGetFuncName(Tfn)
                                             << ": " << nxsGetPropName(prop)
                                             << " = " << name);
            return std::string(name);
          }
          break;
        }
        case NPT_INT_VEC: {
          nxs_long vals[1024];
          size_t size = sizeof(vals);
          if (nxs_success((*fn)(args..., prop, vals, &size))) {
            NEXUS_LOG(NEXUS_STATUS_NOTE, nxsGetFuncName(Tfn)
                                             << ": " << nxsGetPropName(prop)
                                             << " = " << vals);
            std::vector<nxs_long> vec(size / sizeof(nxs_long));
            std::memcpy(vec.data(), vals, size);
            return Property(vec);
          }
          break;
        }
        default: {
          NEXUS_LOG(NEXUS_STATUS_ERR, nxsGetFuncName(Tfn)
                                          << ": Unknown property type for - "
                                          << nxsGetPropName(prop));
          break;
        }
      }
    }
    return std::nullopt;
  }

 private:
  void loadPlugin();

  std::string pluginLibraryPath;
  void *library;
  void *runtimeFns[NXS_FUNCTION_CNT];

  Objects<Device> devices;
};

}  // namespace detail
}  // namespace nexus

#undef NEXUS_LOG_MODULE

#endif  // _NEXUS_RUNTIME_IMPL_H
