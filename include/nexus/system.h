#ifndef NEXUS_SYSTEM_H
#define NEXUS_SYSTEM_H

#include <nexus/buffer.h>
#include <nexus/info.h>
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

  std::optional<Property> getProperty(nxs_int prop) const override;

  Runtimes getRuntimes() const;
  Infos getCatalogs() const;
  Buffers getBuffers() const;

  Runtime getRuntime(int idx) const;
  Runtime getRuntime(const std::string &name);
  Buffer createBuffer(size_t sz, const void *hostData = nullptr,
                      nxs_uint settings = 0);
  Buffer copyBuffer(Buffer buf, Device dev, nxs_uint settings = 0);
  Info loadCatalog(const std::string &catalogPath);
};

extern System getSystem();
}  // namespace nexus

#endif  // NEXUS_SYSTEM_H