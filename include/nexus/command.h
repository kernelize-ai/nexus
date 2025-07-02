#ifndef NEXUS_COMMAND_H
#define NEXUS_COMMAND_H

#include <nexus-api.h>
#include <nexus/buffer.h>
#include <nexus/kernel.h>
#include <nexus/object.h>

#include <functional>
#include <list>

namespace nexus {

namespace detail {
class ScheduleImpl;  // owner
class CommandImpl;
}  // namespace detail

// System class
class Command : public Object<detail::CommandImpl, detail::ScheduleImpl> {
  friend OwnerTy;

 public:
  Command(detail::Impl owner, Kernel kern);
  using Object::Object;

  nxs_int getId() const override;

  std::optional<Property> getProperty(nxs_int prop) const override;

  nxs_status setArgument(nxs_uint index, Buffer buffer) const;
  // nxs_status setArgument(nxs_uint index, nxs_int scalar) const;

  nxs_status finalize(nxs_int gridSize, nxs_int groupSize);
};

typedef Objects<Command> Commands;

class Event : public Command {
 public:
 typedef std::function<void(nxs_status)> Callback;

  Event(detail::Impl owner);

  nxs_status setCallback(Callback callback);

  nxs_status wait();
};

}  // namespace nexus

#endif  // NEXUS_COMMAND_H