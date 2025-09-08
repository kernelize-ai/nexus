#ifndef _NEXUS_LIBRARY_IMPL_H
#define _NEXUS_LIBRARY_IMPL_H

#include <nexus/info.h>
#include <nexus/kernel.h>
#include <nexus/library.h>

#include <unordered_map>

namespace nexus {
namespace detail {

class LibraryImpl : public Impl {
 public:
  /// @brief Construct a Platform for the current system
  LibraryImpl(Impl base);
  LibraryImpl(Impl base, Info info);

  ~LibraryImpl();

  void release();

  std::optional<Property> getProperty(nxs_int prop) const;

  Info getInfo() const { return info; }

  Kernel getKernel(const std::string &kernelName, Info info);

  Kernels getKernels() const { return kernels; }

 private:
  Kernels kernels;
  std::unordered_map<std::string, Kernel> kernelMap;
  Info info;
};
}  // namespace detail
}  // namespace nexus

#endif  // _NEXUS_LIBRARY_IMPL_H