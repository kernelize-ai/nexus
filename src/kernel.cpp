
#include <nexus/kernel.h>
#include <nexus/log.h>

#include "_library_impl.h"

#define NEXUS_LOG_MODULE "kernel"

using namespace nexus;

namespace nexus {
namespace detail {
class KernelImpl : public Impl {
 public:
  /// @brief Construct a Platform for the current system
  KernelImpl(Impl base, const std::string &kName)
      : Impl(base), kernelName(kName) {
    NEXUS_LOG(NEXUS_STATUS_NOTE,
              "  Kernel: " << kernelName << " - " << getId());
  }

  KernelImpl(Impl base, const Info &info) : Impl(base), info(info) {
    // kernelName = info.getProperty(NP_Name).getValue<NP_Name>();
    NEXUS_LOG(NEXUS_STATUS_NOTE,
              "  Kernel: " << kernelName << " - " << getId());
  }

  ~KernelImpl() {
    NEXUS_LOG(NEXUS_STATUS_NOTE, "  ~Kernel: " << getId());
    release();
  }

  void release() {
    auto *rt = getParentOfType<RuntimeImpl>();
    //nxs_int kid = rt->runAPIFunction<NF_nxsReleaseKernel>(getId());
  }

  Info getInfo() const { return info; }

  std::optional<Property> getProperty(nxs_int prop) const {
    auto *rt = getParentOfType<RuntimeImpl>();
    return rt->getAPIProperty<NF_nxsGetKernelProperty>(prop, getId());
  }

 private:
  std::string kernelName;
  Info info;
};
}  // namespace detail
}  // namespace nexus

///////////////////////////////////////////////////////////////////////////////
Kernel::Kernel(detail::Impl base, const std::string &kernelName)
    : Object(base, kernelName) {}

Kernel::Kernel(detail::Impl base, const Info &info)
    : Object(base, info) {}

Info Kernel::getInfo() const { NEXUS_OBJ_MCALL(Info(), getInfo); }

std::optional<Property> Kernel::getProperty(nxs_int prop) const {
  NEXUS_OBJ_MCALL(std::nullopt, getProperty, prop);
}
