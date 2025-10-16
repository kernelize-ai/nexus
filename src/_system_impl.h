
#ifndef _NEXUS_SYSTEM_IMPL_H
#define _NEXUS_SYSTEM_IMPL_H

#include <nexus/buffer.h>
#include <nexus/info.h>
#include <nexus/log.h>
#include <nexus/runtime.h>

namespace nexus {
namespace detail {

class SystemImpl : public detail::Impl {
 public:
  SystemImpl(int);
  ~SystemImpl();

  std::optional<Property> getProperty(nxs_int prop) const;

  Runtime getRuntime(int idx) const { return runtimes.get(idx); }
  Runtime getRuntime(const std::string &name) { 
    auto it = runtimeMap.find(name);
    if (it != runtimeMap.end())
      return it->second;
    return Runtime();
  }
  Buffer createBuffer(size_t sz, const void *hostData = nullptr,
                      nxs_uint options = 0);
  Buffer copyBuffer(Buffer buf, Device dev, nxs_uint options = 0);
  Info loadCatalog(const std::string &catalogPath);

  Runtimes getRuntimes() const { return runtimes; }
  Infos getCatalogs() const { return catalogs; }
  Buffers getBuffers() const { return buffers; }
  
 private:
  // set of runtimes
  Runtimes runtimes;
  std::unordered_map<std::string, Runtime> runtimeMap;
  Infos catalogs;
  Buffers buffers;
};
}  // namespace detail
}  // namespace nexus

#endif  // _NEXUS_SYSTEM_IMPL_H
