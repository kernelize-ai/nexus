#ifndef _NEXUS_LIBRARY_IMPL_H
#define _NEXUS_LIBRARY_IMPL_H

#include <nexus/kernel.h>
#include <nexus/library.h>

#include "_device_impl.h"

namespace nexus {
namespace detail {

class LibraryImpl : public Impl {
 public:
  /// @brief Construct a Platform for the current system
  LibraryImpl(Impl base);

  ~LibraryImpl();

  void release();

  std::optional<Property> getProperty(nxs_int prop) const;

  Kernel getKernel(const std::string &kernelName);

 private:
  Objects<Kernel> kernels;
};
}  // namespace detail
}  // namespace nexus

#endif  // _NEXUS_LIBRARY_IMPL_H