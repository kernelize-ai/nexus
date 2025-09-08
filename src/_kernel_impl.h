#ifndef _NEXUS_KERNEL_IMPL_H
#define _NEXUS_KERNEL_IMPL_H

#include <nexus/info.h>
#include <nexus/kernel.h>

namespace nexus {
namespace detail {

class KernelImpl : public Impl {
 public:
  /// @brief Construct a Platform for the current system
  KernelImpl(Impl base, const std::string &kName, Info info);

  ~KernelImpl();

  void release();

  std::optional<Property> getProperty(nxs_int prop) const;

  Info getInfo() const { return info; }

 private:
  std::string kernelName;
  Info info;
};
}  // namespace detail
}  // namespace nexus

#endif  // _NEXUS_KERNEL_IMPL_H