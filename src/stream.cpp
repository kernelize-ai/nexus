
#include <nexus/log.h>
#include <nexus/stream.h>

#include "_device_impl.h"

#define NEXUS_LOG_MODULE "stream"

namespace nexus {
namespace detail {

class StreamImpl : public Impl {
 public:
  /// @brief Construct a Platform for the current system
  StreamImpl(detail::Impl base) : detail::Impl(base) {
    NEXUS_LOG(NEXUS_STATUS_NOTE, "  Stream: " << getId());
  }

  ~StreamImpl() {
    NEXUS_LOG(NEXUS_STATUS_NOTE, "  ~Stream: " << getId());
    release();
  }

  void release() {
    auto *rt = getParentOfType<RuntimeImpl>();
    nxs_int kid = rt->runAPIFunction<NF_nxsReleaseStream>(getId());
  }

  std::optional<Property> getProperty(nxs_int prop) const {
    auto *rt = getParentOfType<RuntimeImpl>();
    return rt->getAPIProperty<NF_nxsGetStreamProperty>(prop, getId());
  }

 private:
};
}  // namespace detail
}  // namespace nexus

using namespace nexus;
using namespace nexus::detail;

///////////////////////////////////////////////////////////////////////////////
Stream::Stream(detail::Impl base) : Object(base) {}

std::optional<Property> Stream::getProperty(nxs_int prop) const {
  NEXUS_OBJ_MCALL(std::nullopt, getProperty, prop);
}
