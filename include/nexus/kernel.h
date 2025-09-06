#ifndef NEXUS_KERNEL_H
#define NEXUS_KERNEL_H

#include <nexus-api.h>
#include <nexus/object.h>
#include <nexus/info.h>

namespace nexus {

namespace detail {
class KernelImpl;
}  // namespace detail

// System class
class Kernel : public Object<detail::KernelImpl> {
 public:
  Kernel(detail::Impl base, const std::string &kernelName, Info info = Info());
  using Object::Object;

  Info getInfo() const;

  std::optional<Property> getProperty(nxs_int prop) const override;
};

typedef Objects<Kernel> Kernels;

}  // namespace nexus

#endif  // NEXUS_KERNEL_H