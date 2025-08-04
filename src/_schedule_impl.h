
#ifndef _NEXUS_SCHEDULE_IMPL_H
#define _NEXUS_SCHEDULE_IMPL_H

#include "_device_impl.h"

namespace nexus {
namespace detail {

class ScheduleImpl : public Impl {
 public:
  /// @brief Construct a Platform for the current system
  ScheduleImpl(Impl base);
  ~ScheduleImpl();

  void release();

  std::optional<Property> getProperty(nxs_int prop) const;

  Command createCommand(Kernel kern, nxs_uint settings);
  Command createCommand(Event event, nxs_uint settings);
  Command createSignalCommand(nxs_int signal_value, nxs_uint settings);
  Command createSignalCommand(Event event, nxs_int signal_value,
                              nxs_uint settings);
  Command createWaitCommand(Event event, nxs_int wait_value, nxs_uint settings);

  nxs_status run(Stream stream, nxs_uint settings);

 private:
  Commands commands;
};
}  // namespace detail
}  // namespace nexus

#endif  // _NEXUS_SCHEDULE_IMPL_H
