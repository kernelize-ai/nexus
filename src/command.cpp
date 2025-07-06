
#include <nexus/command.h>
#include <nexus/log.h>

#include "_schedule_impl.h"

#define NEXUS_LOG_MODULE "command"

using namespace nexus;

namespace nexus {
namespace detail {
class CommandImpl : public Impl {
 public:
  /// @brief Construct a Platform for the current system
  CommandImpl(Impl owner, Kernel kern) : Impl(owner), kernel(kern) {
    NEXUS_LOG(NEXUS_STATUS_NOTE, "    Command: " << getId());
    arguments.reserve(16); // TODO: get from kernel
    // TODO: gather kernel argument details
  }

  CommandImpl(Impl owner, Event event) : Impl(owner), event(event) {
    NEXUS_LOG(NEXUS_STATUS_NOTE, "    Command: " << getId());
  }

  ~CommandImpl() {
    NEXUS_LOG(NEXUS_STATUS_NOTE, "    ~Command: " << getId());
    release();
  }

  void release() {
    auto *rt = getParentOfType<RuntimeImpl>();
    nxs_int kid = rt->runAPIFunction<NF_nxsReleaseCommand>(getId());
    arguments.clear();
  }

  std::optional<Property> getProperty(nxs_int prop) const {
    return std::nullopt;
  }

  Kernel getKernel() const { return kernel; }
  Event getEvent() const { return event; }

  nxs_status setArgument(nxs_uint index, Buffer buffer) {
    if (event) return NXS_InvalidArgIndex;
    addArgument(index, buffer);
    auto *rt = getParentOfType<RuntimeImpl>();
    return (nxs_status)rt->runAPIFunction<NF_nxsSetCommandArgument>(
        getId(), index, buffer.getId());
  }

  nxs_status finalize(nxs_int groupSize, nxs_int gridSize) {
    if (event) return NXS_InvalidArgIndex;
    auto *rt = getParentOfType<RuntimeImpl>();
    return (nxs_status)rt->runAPIFunction<NF_nxsFinalizeCommand>(
        getId(), groupSize, gridSize);
  }

 private:
  Kernel kernel;
  Event event;

  void addArgument(nxs_uint index, Buffer buffer) {
    if (index >= arguments.size())
      arguments.resize(index + 1);
    arguments[index] = buffer;
  }

  std::vector<Buffer> arguments;
};
}  // namespace detail
}  // namespace nexus

///////////////////////////////////////////////////////////////////////////////
Command::Command(detail::Impl owner, Kernel kern) : Object(owner, kern) {}

Command::Command(detail::Impl owner, Event event) : Object(owner, event) {}

nxs_int Command::getId() const { NEXUS_OBJ_MCALL(NXS_InvalidCommand, getId); }

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

nxs_status Command::finalize(nxs_int groupSize, nxs_int gridSize) {
  NEXUS_OBJ_MCALL(NXS_InvalidCommand, finalize, groupSize, gridSize);
}
