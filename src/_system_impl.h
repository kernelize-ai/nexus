
#ifndef _NEXUS_SYSTEM_IMPL_H
#define _NEXUS_SYSTEM_IMPL_H

#include <nexus/buffer.h>
#include <nexus/runtime.h>

namespace nexus {
namespace detail {

class SystemImpl : public detail::Impl {
 public:
  SystemImpl(int);
  ~SystemImpl();

  std::optional<Property> getProperty(nxs_int prop) const;

  Runtime getRuntime(int idx) const { return runtimes.get(idx); }
  Runtime getRuntime(std::string name) const { return runtimes.get(name); }

  Buffer createBuffer(size_t sz, const void *hostData = nullptr);
  Buffer copyBuffer(Buffer buf, Device dev);

  Runtimes getRuntimes() const { return runtimes; }

  Buffers getBuffers() const { return buffers; }

 private:
  // set of runtimes
  Runtimes runtimes;
  Buffers buffers;
};
}  // namespace detail
}  // namespace nexus

#endif  // _NEXUS_SYSTEM_IMPL_H
