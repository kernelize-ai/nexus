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
  Command(detail::Impl base, Kernel kern);
  Command(detail::Impl base, Event event);
  using Object::Object;

  std::optional<Property> getProperty(nxs_int prop) const override;

  Kernel getKernel() const;
  Event getEvent() const;

  nxs_status setArgument(nxs_uint index, Buffer buffer);
  nxs_status setArgument(nxs_uint index, nxs_int value);
  nxs_status setArgument(nxs_uint index, nxs_uint value);
  nxs_status setArgument(nxs_uint index, nxs_long value);
  nxs_status setArgument(nxs_uint index, nxs_ulong value);
  nxs_status setArgument(nxs_uint index, nxs_float value);
  nxs_status setArgument(nxs_uint index, nxs_double value);

  nxs_status finalize(nxs_dim3 gridSize, nxs_dim3 groupSize, nxs_uint shmemSize);
};

typedef Objects<Command> Commands;

}  // namespace nexus

#endif  // NEXUS_COMMAND_H