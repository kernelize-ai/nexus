
#include <nexus/command.h>
#include <nexus/log.h>

#include "_schedule_impl.h"

#define NEXUS_LOG_MODULE "command"

using namespace nexus;

namespace nexus {
namespace detail {
class CommandImpl : public Impl {
  typedef std::variant<Buffer, nxs_int, nxs_uint, nxs_long, nxs_ulong,
                       nxs_float, nxs_double, nxs_short, nxs_ushort, nxs_char, nxs_uchar, bool>
      Arg;

  struct ArgValue {
    Arg value;
    std::string name;
  };

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
    auto *rt = getParentOfType<RuntimeImpl>();
    if (settings & NXS_CommandArgType_Constant) {
      void *val_ptr = putConstant(index, value, name);
      const char *name_ptr = constants[index].name.c_str();
      return (nxs_status)rt->runAPIFunction<NF_nxsSetCommandScalar>(
          getId(), index, val_ptr, name_ptr, settings);
    }
    void *val_ptr = putArgument(index, value, name);
    const char *name_ptr = arguments[index].name.c_str();
    return (nxs_status)rt->runAPIFunction<NF_nxsSetCommandScalar>(
        getId(), index, val_ptr, name_ptr, settings);
  }

  nxs_status setArgument(nxs_uint index, Buffer buffer, const char *name, nxs_uint settings) {
    if (event) return NXS_InvalidArgIndex;
    putArgument(index, buffer, name);
    auto *rt = getParentOfType<RuntimeImpl>();
    return (nxs_status)rt->runAPIFunction<NF_nxsSetCommandArgument>(
        getId(), index, buffer.getId(), arguments[index].name.c_str(), settings);
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
  T *putArgument(nxs_uint index, T value, const char *name) {
    if (index >= arguments.size())
      return nullptr;
    arguments[index] = {value, name};
    return &std::get<T>(arguments[index].value);
  }

  template <typename T>
  T *putConstant(nxs_uint index, T value, const char *name) {
    if (index >= NXS_KERNEL_MAX_CONSTS)
      return nullptr;
    constants[index] = {value, name};
    return &std::get<T>(constants[index].value);
  }

  std::array<ArgValue, NXS_KERNEL_MAX_ARGS> arguments;
  std::array<ArgValue, NXS_KERNEL_MAX_CONSTS> constants;
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

template <>
nxs_status Command::setArgument<Buffer>(nxs_uint index, Buffer buffer, const char *name, nxs_uint settings) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setArgument, index, buffer, name, settings);
}

template <>
nxs_status Command::setArgument<nxs_int>(nxs_uint index, nxs_int value, const char *name, nxs_uint settings) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setScalar, index, value, name, settings);
}

template <>
nxs_status Command::setArgument<nxs_uint>(nxs_uint index, nxs_uint value, const char *name, nxs_uint settings) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setScalar, index, value, name, settings);
}

template <>
nxs_status Command::setArgument<nxs_long>(nxs_uint index, nxs_long value, const char *name, nxs_uint settings) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setScalar, index, value, name, settings);
}

template <>
nxs_status Command::setArgument<nxs_ulong>(nxs_uint index, nxs_ulong value, const char *name, nxs_uint settings) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setScalar, index, value, name, settings);
}

template <>
nxs_status Command::setArgument<nxs_float>(nxs_uint index, nxs_float value, const char *name, nxs_uint settings) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setScalar, index, value, name, settings);
}

template <>
nxs_status Command::setArgument<nxs_double>(nxs_uint index, nxs_double value, const char *name, nxs_uint settings) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setScalar, index, value, name, settings);
}

template <>
nxs_status Command::setArgument<nxs_short>(nxs_uint index, nxs_short value, const char *name, nxs_uint settings) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setScalar, index, value, name, settings);
}

template <>
nxs_status Command::setArgument<nxs_ushort>(nxs_uint index, nxs_ushort value, const char *name, nxs_uint settings) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setScalar, index, value, name, settings);
}

template <>
nxs_status Command::setArgument<nxs_char>(nxs_uint index, nxs_char value, const char *name, nxs_uint settings) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setScalar, index, value, name, settings);
}

template <>
nxs_status Command::setArgument<nxs_uchar>(nxs_uint index, nxs_uchar value, const char *name, nxs_uint settings) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setScalar, index, value, name, settings);
}

template <>
nxs_status Command::setArgument<bool>(nxs_uint index, bool value, const char *name, nxs_uint settings) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, setScalar, index, value, name, settings);
}

nxs_status Command::finalize(nxs_dim3 gridSize, nxs_dim3 groupSize, nxs_uint sharedMemorySize) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, finalize, gridSize, groupSize, sharedMemorySize);
}
