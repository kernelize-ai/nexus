
#include <nexus/command.h>
#include <nexus/log.h>

#include "_schedule_impl.h"

#define NEXUS_LOG_MODULE "command"

using namespace nexus;

namespace nexus {
namespace detail {
class CommandImpl : public Impl {
  typedef std::variant<Buffer, nxs_int, nxs_uint, nxs_long, nxs_ulong,
                       nxs_float, nxs_double, nxs_half, nxs_short, nxs_char>
      Arg;

 public:
  /// @brief Construct a Platform for the current system
  CommandImpl(Impl owner, Kernel kern) : Impl(owner), kernel(kern) {
    NEXUS_LOG(NXS_LOG_NOTE, "    Command: ", getId());
    // TODO: gather kernel argument details
  }

  CommandImpl(Impl owner, Event event) : Impl(owner), event(event) {
    NEXUS_LOG(NXS_LOG_NOTE, "    Command: ", getId());
  }

  ~CommandImpl() {
    NEXUS_LOG(NXS_LOG_NOTE, "    ~Command: ", getId());
    release();
  }

  void release() {}

  std::optional<Property> getProperty(nxs_int prop) const {
    auto *rt = getParentOfType<RuntimeImpl>();
    return rt->getAPIProperty<NF_nxsGetCommandProperty>(prop, getId());
  }

  Kernel getKernel() const { return kernel; }
  Event getEvent() const { return event; }

  template <typename T>
  nxs_status setScalar(nxs_uint index, T value, const char *name, nxs_uint settings) {
    if (event) return NXS_InvalidArgIndex;
    void *val_ptr;
    if (settings & NXS_CommandArgType_Constant)
      val_ptr = putConstant(index, value);
    else
      val_ptr = putArgument(index, value);
    auto *rt = getParentOfType<RuntimeImpl>();
    return (nxs_status)rt->runAPIFunction<NF_nxsSetCommandScalar>(
        getId(), index, val_ptr, name, settings);
  }

  nxs_status setArgument(nxs_uint index, Buffer buffer, const char *name, nxs_uint settings) {
    if (event) return NXS_InvalidArgIndex;
    putArgument(index, buffer);
    auto *rt = getParentOfType<RuntimeImpl>();
    return (nxs_status)rt->runAPIFunction<NF_nxsSetCommandArgument>(
        getId(), index, buffer.getId(), name, settings);
  }

  nxs_status finalize(nxs_dim3 gridSize, nxs_dim3 groupSize, nxs_uint sharedMemorySize) {
    if (event) return NXS_InvalidArgIndex;
    auto *rt = getParentOfType<RuntimeImpl>();
    return (nxs_status)rt->runAPIFunction<NF_nxsFinalizeCommand>(
        getId(), gridSize, groupSize, sharedMemorySize);
  }

 private:
  Kernel kernel;
  Event event;

  template <typename T>
  T *putArgument(nxs_uint index, T value) {
    if (index >= arguments.size())
      return nullptr;
    arguments[index] = value;
    return &std::get<T>(arguments[index]);
  }

  template <typename T>
  T *putConstant(nxs_uint index, T value) {
    if (index >= NXS_KERNEL_MAX_CONSTS)
      return nullptr;
    arguments[index] = value;
    return &std::get<T>(arguments[index]);
  }

  std::array<Arg, NXS_KERNEL_MAX_ARGS> arguments;
  std::array<Arg, NXS_KERNEL_MAX_CONSTS> constants;
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

nxs_status Command::setArgument(nxs_uint index, Buffer buffer, const char *name, nxs_uint settings) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setArgument, index, buffer, name, settings);
}

nxs_status Command::setArgument(nxs_uint index, nxs_int value, const char *name, nxs_uint settings) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setScalar, index, value, name, settings);
}

nxs_status Command::setArgument(nxs_uint index, nxs_uint value, const char *name, nxs_uint settings) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setScalar, index, value, name, settings);
}

nxs_status Command::setArgument(nxs_uint index, nxs_long value, const char *name, nxs_uint settings) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setScalar, index, value, name, settings);
}

nxs_status Command::setArgument(nxs_uint index, nxs_ulong value, const char *name, nxs_uint settings) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setScalar, index, value, name, settings);
}

nxs_status Command::setArgument(nxs_uint index, nxs_float value, const char *name, nxs_uint settings) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setScalar, index, value, name, settings);
}

nxs_status Command::setArgument(nxs_uint index, nxs_double value, const char *name, nxs_uint settings) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setScalar, index, value, name, settings);
}

nxs_status Command::finalize(nxs_dim3 gridSize, nxs_dim3 groupSize, nxs_uint sharedMemorySize) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, finalize, gridSize, groupSize, sharedMemorySize);
}
