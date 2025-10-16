
#include <nexus/kernel.h>
#include <nexus/log.h>

#include "_kernel_impl.h"
#include "_runtime_impl.h"

#define NEXUS_LOG_MODULE "kernel"

using namespace nexus;

namespace nexus {
namespace detail {

  /// @brief Construct a Platform for the current system
KernelImpl::KernelImpl(Impl base, const std::string &kName, Info info)
    : Impl(base), kernelName(kName), info(info) {
  NEXUS_LOG(NXS_LOG_NOTE, "  Kernel: ", kernelName, " - ", getId());
}

KernelImpl::~KernelImpl() {
  NEXUS_LOG(NXS_LOG_NOTE, "  ~Kernel: ", getId());
  release();
}

void KernelImpl::release() {
  auto *rt = getParentOfType<RuntimeImpl>();
  // nxs_int kid = rt->runAPIFunction<NF_nxsReleaseKernel>(getId());
}

std::optional<Property> KernelImpl::getProperty(nxs_int prop) const {
  auto *rt = getParentOfType<RuntimeImpl>();
  return rt->getAPIProperty<NF_nxsGetKernelProperty>(prop, getId());
}
}  // namespace detail
}  // namespace nexus

///////////////////////////////////////////////////////////////////////////////
Kernel::Kernel(detail::Impl base, const std::string &kernelName, Info info)
    : Object(base, kernelName, info) {}

Info Kernel::getInfo() const { NEXUS_OBJ_MCALL(Info(), getInfo); }

std::optional<Property> Kernel::getProperty(nxs_int prop) const {
  NEXUS_OBJ_MCALL(std::nullopt, getProperty, prop);
}
