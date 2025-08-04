
#include <nexus/command.h>
#include <nexus/log.h>

#include "_schedule_impl.h"

#define NEXUS_LOG_MODULE "command"

using namespace nexus;

namespace nexus {
namespace detail {
class CommandImpl : public Impl {
  typedef std::variant<Buffer, nxs_int, nxs_uint, nxs_long, nxs_ulong,
                       nxs_float, nxs_double>
      Arg;

 public:
  /// @brief Construct a Platform for the current system
  CommandImpl(Impl owner, Kernel kern) : Impl(owner), kernel(kern) {
    NEXUS_LOG(NEXUS_STATUS_NOTE, "    Command: " << getId());
    arguments.reserve(32);  // TODO: get from kernel
    // TODO: gather kernel argument details
  }

  CommandImpl(Impl owner, Event event) : Impl(owner), event(event) {
    NEXUS_LOG(NEXUS_STATUS_NOTE, "    Command: " << getId());
  }

  ~CommandImpl() {
    NEXUS_LOG(NEXUS_STATUS_NOTE, "    ~Command: " << getId());
    release();
  }

  void release() { arguments.clear(); }

  std::optional<Property> getProperty(nxs_int prop) const {
    auto *rt = getParentOfType<RuntimeImpl>();
    return rt->getAPIProperty<NF_nxsGetCommandProperty>(prop, getId());
  }

  Kernel getKernel() const { return kernel; }
  Event getEvent() const { return event; }

  template <typename T>
  nxs_status setScalar(nxs_uint index, T value) {
    if (event) return NXS_InvalidArgIndex;
    void *val_ptr = putArgument(index, value);
    auto *rt = getParentOfType<RuntimeImpl>();
    return (nxs_status)rt->runAPIFunction<NF_nxsSetCommandScalar>(
        getId(), index, val_ptr);
  }

  nxs_status setArgument(nxs_uint index, Buffer buffer) {
    if (event) return NXS_InvalidArgIndex;
    putArgument(index, buffer);
    auto *rt = getParentOfType<RuntimeImpl>();
    return (nxs_status)rt->runAPIFunction<NF_nxsSetCommandArgument>(
        getId(), index, buffer.getId());
  }

  nxs_status finalize(nxs_int gridSize, nxs_int groupSize) {
    if (event) return NXS_InvalidArgIndex;
    auto *rt = getParentOfType<RuntimeImpl>();
    return (nxs_status)rt->runAPIFunction<NF_nxsFinalizeCommand>(
        getId(), gridSize, groupSize);
  }

 private:
  Kernel kernel;
  Event event;

  template <typename T>
  T *putArgument(nxs_uint index, T value) {
    if (index >= arguments.size())
      arguments.resize(index + 1);
    arguments[index] = value;
    return &std::get<T>(arguments[index]);
  }

  std::vector<Arg> arguments;
};
}  // namespace detail
}  // namespace nexus

///////////////////////////////////////////////////////////////////////////////
Command::Command(detail::Impl base, Kernel kern) : Object(base, kern) {}

Command::Command(detail::Impl base, Event event) : Object(base, event) {}

std::optional<Property> Command::getProperty(nxs_int prop) const {
  NEXUS_OBJ_MCALL(std::nullopt, getProperty, prop);
}

Kernel Command::getKernel() const {
  NEXUS_OBJ_MCALL(Kernel(), getKernel);
}

Event Command::getEvent() const {
  NEXUS_OBJ_MCALL(Event(), getEvent);
}

nxs_status Command::setArgument(nxs_uint index, Buffer buffer) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setArgument, index, buffer);
}

nxs_status Command::setArgument(nxs_uint index, nxs_int value) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setScalar, index, value);
}

nxs_status Command::setArgument(nxs_uint index, nxs_uint value) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setScalar, index, value);
}

nxs_status Command::setArgument(nxs_uint index, nxs_long value) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setScalar, index, value);
}

nxs_status Command::setArgument(nxs_uint index, nxs_ulong value) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setScalar, index, value);
}

nxs_status Command::setArgument(nxs_uint index, nxs_float value) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setScalar, index, value);
}

nxs_status Command::setArgument(nxs_uint index, nxs_double value) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setScalar, index, value);
}

nxs_status Command::finalize(nxs_int gridSize, nxs_int groupSize) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, finalize, gridSize, groupSize);
}
