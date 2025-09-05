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
  LibraryImpl(Impl base, const Info &info);

  ~LibraryImpl();

  void release();

  std::optional<Property> getProperty(nxs_int prop) const;

  Info getInfo() const { return info; }

  Kernel getKernel(const std::string &kernelName);

 private:
  Objects<Kernel> kernels;
  std::unordered_map<std::string, Kernel> kernelMap;
  Info info;
};
}  // namespace detail
}  // namespace nexus

#endif  // _NEXUS_LIBRARY_IMPL_H