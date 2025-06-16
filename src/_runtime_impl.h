#ifndef _NEXUS_RUNTIME_IMPL_H
#define _NEXUS_RUNTIME_IMPL_H

#include <nexus/device.h>
#include <nexus-api.h>

#include <optional>
#include <string>
#include <memory>

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

            template <nxs_function Tfn, typename Tfnp = typename nxsFunctionType<Tfn>::type>
            Tfnp getFunction() const { return (Tfnp)runtimeFns[Tfn]; }

            // Get Runtime Property Value
            template <typename T>
            const T getProperty(nxs_property pn) const {
                size_t size = sizeof(T);
                T val = 0;
                //assert(typeid(T), typeid(pm_t)); // how to lookup at runtime
                if (auto fn = getFunction<NF_nxsGetRuntimeProperty>())
                    (*fn)(pn, &val, &size);
                return val;
            }

            const std::string getProperty(nxs_property pn) const;

            template <nxs_function Tfn, typename... Args>
            nxs_int runAPIFunction(Args... args) {
                nxs_int apiResult = NXS_InvalidDevice; // invalid runtime
                if (auto *fn = getFunction<Tfn>()) {
                    apiResult = (*fn)(args...);
                    if (nxs_failed(apiResult))
                        NEXUS_LOG(NEXUS_STATUS_ERR, nxsGetFuncName(Tfn) << ": " << nxsGetStatusName(apiResult));
                    else
                        NEXUS_LOG(NEXUS_STATUS_NOTE, nxsGetFuncName(Tfn) << ": " << apiResult);
                } else {
                  NEXUS_LOG(NEXUS_STATUS_ERR, nxsGetFuncName(Tfn) << ": API not present");
                }
                return apiResult;
            }

        private:
            void loadPlugin();

            std::string pluginLibraryPath;
            void * library;
            void * runtimeFns[NXS_FUNCTION_CNT];

            Objects<Device> devices;
        };

    }
}

#undef NEXUS_LOG_MODULE

#endif // _NEXUS_RUNTIME_IMPL_H
