#ifndef NEXUS_EVENT_H
#define NEXUS_EVENT_H

////////////////////////////////////////////////////////////
// Event
// - Event is a synchronization primitive that allows the host to wait for the
//   completion of a command.
// - Event is created by the host and is associated with a command queue.
// - Event is destroyed when the command queue is destroyed.
// - Event is used to synchronize the host and the device.
////////////////////////////////////////////////////////////

#include <nexus-api.h>
#include <nexus/object.h>

namespace nexus {

namespace detail {
class EventImpl;
}  // namespace detail

class Event : public Object<detail::EventImpl> {
 public:
  Event(detail::Impl base, nxs_int value = 1);
  using Object::Object;

  std::optional<Property> getProperty(nxs_int prop) const override;

  nxs_status signal(nxs_int value = 1);
  nxs_status wait(nxs_int value = 1);
};

typedef Objects<Event> Events;

}  // namespace nexus

#endif  // NEXUS_EVENT_H