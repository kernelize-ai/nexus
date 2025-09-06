#ifndef NEXUS_SCHEDULE_H
#define NEXUS_SCHEDULE_H

#include <nexus-api.h>
#include <nexus/command.h>
#include <nexus/stream.h>
#include <nexus/object.h>

namespace nexus {

namespace detail {
class ScheduleImpl;
}  // namespace detail

// System class
class Schedule : public Object<detail::ScheduleImpl> {
 public:
  Schedule(detail::Impl base);
  using Object::Object;

  std::optional<Property> getProperty(nxs_int prop) const override;

  Command createCommand(Kernel kern, nxs_uint settings = 0);
  Command createSignalCommand(nxs_int signal_value = 1, nxs_uint settings = 0);
  Command createSignalCommand(Event event, nxs_int signal_value = 1,
                              nxs_uint settings = 0);
  Command createWaitCommand(Event event, nxs_int wait_value = 1,
                            nxs_uint settings = 0);

  nxs_status run(Stream stream = Stream(), nxs_uint settings = 0);

  Commands getCommands() const;
};

typedef Objects<Schedule> Schedules;

}  // namespace nexus

#endif  // NEXUS_SCHEDULE_H