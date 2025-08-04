#ifndef NEXUS_KERNEL_H
#define NEXUS_KERNEL_H

#include <nexus-api.h>
#include <nexus/object.h>

namespace nexus {

namespace detail {
class KernelImpl;
}  // namespace detail

// System class
class Kernel : public Object<detail::KernelImpl> {
 public:
  Kernel(detail::Impl base, const std::string &kernelName);
  using Object::Object;

  std::optional<Property> getProperty(nxs_int prop) const override;
};

typedef Objects<Kernel> Kernels;

}  // namespace nexus

#endif  // NEXUS_KERNEL_H