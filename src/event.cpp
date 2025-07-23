
#include <nexus/event.h>
#include <nexus/log.h>

#include "_runtime_impl.h"

#define NEXUS_LOG_MODULE "event"

using namespace nexus;

namespace nexus {
namespace detail {
class EventImpl : public Impl {
 public:
  /// @brief Construct a Platform for the current system
  EventImpl(Impl owner, nxs_int value) : Impl(owner), value(value) {
    NEXUS_LOG(NEXUS_STATUS_NOTE, "    Event: " << getId());
  }

  ~EventImpl() {
    NEXUS_LOG(NEXUS_STATUS_NOTE, "    ~Event: " << getId());
    release();
  }

  void release() {
    auto *rt = getParentOfType<RuntimeImpl>();
    nxs_int kid = rt->runAPIFunction<NF_nxsReleaseEvent>(getId());
  }

  std::optional<Property> getProperty(nxs_int prop) const {
    auto *rt = getParentOfType<RuntimeImpl>();
    return rt->getAPIProperty<NF_nxsGetEventProperty>(prop, getId());
  }

  nxs_status signal(nxs_int value) {
    auto *rt = getParentOfType<RuntimeImpl>();
    return (nxs_status)rt->runAPIFunction<NF_nxsSignalEvent>(getId(), value);
  }

  nxs_status wait(nxs_int value) {
    auto *rt = getParentOfType<RuntimeImpl>();
    return (nxs_status)rt->runAPIFunction<NF_nxsWaitEvent>(getId(), value);
  }

 private:
  nxs_int value;
};
}  // namespace detail
}  // namespace nexus

///////////////////////////////////////////////////////////////////////////////
Event::Event(detail::Impl owner, nxs_int value) : Object(owner, value) {}

nxs_int Event::getId() const { NEXUS_OBJ_MCALL(NXS_InvalidEvent, getId); }

std::optional<Property> Event::getProperty(nxs_int prop) const {
  NEXUS_OBJ_MCALL(std::nullopt, getProperty, prop);
}

nxs_status Event::signal(nxs_int value) {
  NEXUS_OBJ_MCALL(NXS_InvalidEvent, signal, value);
}

nxs_status Event::wait(nxs_int value) {
  NEXUS_OBJ_MCALL(NXS_InvalidEvent, wait, value);
}
