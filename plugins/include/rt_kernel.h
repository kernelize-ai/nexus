#ifndef RT_KERNEL_H
#define RT_KERNEL_H

#include <string>
#include <vector>

#include <cuda_runtime.h>
#include <cuda.h>

#include <rt_buffer.h>

namespace nxs {
namespace rt {

class Kernel : public Object {

public:

  std::string name; // The name of the kernel
  std::vector<Buffer> arguments; // Arguments for the kernel

  Kernel() = default;
  Kernel(const std::string &name)
    : name(name) {}
  virtual ~Kernel() = default;
};

}  // namespace rt
}  // namespace nxs

#endif // RT_KERNEL_H