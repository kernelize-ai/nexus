
#ifndef _NEXUS_SCHEDULE_IMPL_H
#define _NEXUS_SCHEDULE_IMPL_H

#include "_device_impl.h"

namespace nexus {
namespace detail {

class ScheduleImpl : public Impl {
 public:
  /// @brief Construct a Platform for the current system
  ScheduleImpl(Impl owner);
  ~ScheduleImpl();

  void release();

  std::optional<Property> getProperty(nxs_int prop) const;

  Command createCommand(Kernel kern);
  Command createCommand(Event event);
  Command createSignalCommand(nxs_int signal_value);
  Command createSignalCommand(Event event, nxs_int signal_value);
  Command createWaitCommand(Event event, nxs_int wait_value);

  nxs_status run(Stream stream, nxs_bool blocking);

 private:
  Commands commands;
};
}  // namespace detail
}  // namespace nexus

#endif  // _NEXUS_SCHEDULE_IMPL_H
