#ifndef NEXUS_COMMAND_H
#define NEXUS_COMMAND_H

#include <nexus-api.h>
#include <nexus/buffer.h>
#include <nexus/event.h>
#include <nexus/kernel.h>
#include <nexus/object.h>

#include <functional>
#include <list>

namespace nexus {

namespace detail {
class CommandImpl;
}  // namespace detail

// System class
class Command : public Object<detail::CommandImpl> {
 public:
  Command(detail::Impl owner, Kernel kern);
  Command(detail::Impl owner, Event event);
  using Object::Object;

  nxs_int getId() const override;

  std::optional<Property> getProperty(nxs_int prop) const override;

  Kernel getKernel() const;
  Event getEvent() const;

  nxs_status setArgument(nxs_uint index, Buffer buffer);

  // scalar arguments
  template <typename T>
  nxs_status setArgument(nxs_uint index, T value);

  nxs_status finalize(nxs_int gridSize, nxs_int groupSize);
};

typedef Objects<Command> Commands;

}  // namespace nexus

#endif  // NEXUS_COMMAND_H