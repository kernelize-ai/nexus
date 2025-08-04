#ifndef NEXUS_LIBRARY_H
#define NEXUS_LIBRARY_H

#include <nexus-api.h>
#include <nexus/kernel.h>
#include <nexus/object.h>

namespace nexus {

namespace detail {
class LibraryImpl;
}  // namespace detail

// System class
class Library : public Object<detail::LibraryImpl> {
 public:
  Library(detail::Impl base);
  using Object::Object;

  std::optional<Property> getProperty(nxs_int prop) const override;

  Kernel getKernel(const std::string &kernelName);
};

typedef Objects<Library> Librarys;

}  // namespace nexus

#endif  // NEXUS_LIBRARY_H