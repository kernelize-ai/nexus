#ifndef NEXUS_SYSTEM_H
#define NEXUS_SYSTEM_H

#include <nexus/buffer.h>
#include <nexus/runtime.h>

#include <memory>
#include <optional>
#include <vector>

namespace nexus {
namespace detail {
class SystemImpl;
}

// System class
class System : Object<detail::SystemImpl> {
 public:
  System(int);
  using Object::Object;

  nxs_int getId() const override { return 0; }

  std::optional<Property> getProperty(nxs_int prop) const override;

  Runtimes getRuntimes() const;
  Buffers getBuffers() const;

  Runtime getRuntime(int idx) const;
  Runtime getRuntime(const std::string &name);
  Buffer createBuffer(size_t sz, const void *hostData = nullptr);
  Buffer copyBuffer(Buffer buf, Device dev);
};

extern System getSystem();
}  // namespace nexus

#endif  // NEXUS_SYSTEM_H