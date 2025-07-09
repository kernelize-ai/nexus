#ifndef RT_LIBRARY_H
#define RT_LIBRARY_H

#include <string>
#include <vector>

#include <nexus-api.h>

#include <rt_kernel.h>

namespace nxs {
namespace rt {

class Library : public Object {

public:

  Kernel kernel;
  size_t kernelSize;

  Library() = default;
  Library(nxs_uint data_size) {}
  ~Library() = default;

};

}  // namespace rt
}  // namespace nxs

#endif // RT_LIBRARY_H