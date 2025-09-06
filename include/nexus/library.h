#ifndef NEXUS_LIBRARY_H
#define NEXUS_LIBRARY_H

#include <nexus-api.h>
#include <nexus/kernel.h>
#include <nexus/object.h>
#include <nexus/info.h>

namespace nexus {

namespace detail {
class LibraryImpl;
}  // namespace detail

// System class
class Library : public Object<detail::LibraryImpl> {
 public:
  Library(detail::Impl base);
  Library(detail::Impl base, Info info);
  using Object::Object;

  Info getInfo() const;

  std::optional<Property> getProperty(nxs_int prop) const override;

  Kernel getKernel(const std::string &kernelName, Info info = Info());

  Kernels getKernels() const;
};

typedef Objects<Library> Librarys;

}  // namespace nexus

#endif  // NEXUS_LIBRARY_H