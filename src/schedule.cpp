
#include <nexus/command.h>
#include <nexus/log.h>
#include <nexus/schedule.h>

#include "_schedule_impl.h"

#define NEXUS_LOG_MODULE "schedule"

using namespace nexus;
using namespace nexus::detail;

/// @brief Construct a Platform for the current system
ScheduleImpl::ScheduleImpl(detail::Impl owner) : detail::Impl(owner) {
  NEXUS_LOG(NEXUS_STATUS_NOTE, "  Schedule: " << getId());
}

ScheduleImpl::~ScheduleImpl() {
  NEXUS_LOG(NEXUS_STATUS_NOTE, "  ~Schedule: " << getId());
  release();
}

void ScheduleImpl::release() {
  commands.clear();
  auto *rt = getParentOfType<RuntimeImpl>();
  nxs_int kid = rt->runAPIFunction<NF_nxsReleaseSchedule>(getId());
}

std::optional<Property> ScheduleImpl::getProperty(nxs_int prop) const {
  auto *rt = getParentOfType<RuntimeImpl>();
  return rt->getAPIProperty<NF_nxsGetScheduleProperty>(prop, getId());
}

Command ScheduleImpl::createCommand(Kernel kern) {
  auto *rt = getParentOfType<RuntimeImpl>();
  nxs_int cid = rt->runAPIFunction<NF_nxsCreateCommand>(getId(), kern.getId());
  Command cmd(detail::Impl(this, cid), kern);
  commands.add(cmd);
  return cmd;
}

Command ScheduleImpl::createSignalCommand(nxs_int signal_value) {
  auto *dev = getParentOfType<DeviceImpl>();
  Event event = dev->createEvent();
  auto *rt = getParentOfType<RuntimeImpl>();
  nxs_int cid = rt->runAPIFunction<NF_nxsCreateSignalCommand>(getId(), event.getId(), signal_value);
  Command cmd(detail::Impl(this, cid), event);
  commands.add(cmd);
  return cmd;
}

Command ScheduleImpl::createSignalCommand(Event event, nxs_int signal_value) {
  if (!event) {
    auto *dev = getParentOfType<DeviceImpl>();
    event = dev->createEvent();
  }
  auto *rt = getParentOfType<RuntimeImpl>();
  nxs_int cid = rt->runAPIFunction<NF_nxsCreateSignalCommand>(getId(), event.getId(), signal_value);
  Command cmd(detail::Impl(this, cid), event);
  commands.add(cmd);
  return cmd;
}

Command ScheduleImpl::createWaitCommand(Event event, nxs_int wait_value) {
  auto *rt = getParentOfType<RuntimeImpl>();
  nxs_int cid = rt->runAPIFunction<NF_nxsCreateWaitCommand>(getId(), event.getId(), wait_value);
  Command cmd(detail::Impl(this, cid), event);
  commands.add(cmd);
  return cmd;
}

nxs_status ScheduleImpl::run(Stream stream, nxs_bool blocking) {
  auto *rt = getParentOfType<RuntimeImpl>();
  return (nxs_status)rt->runAPIFunction<NF_nxsRunSchedule>(getId(), stream.getId(), blocking);
}

///////////////////////////////////////////////////////////////////////////////
Schedule::Schedule(detail::Impl owner) : Object(owner) {}

nxs_int Schedule::getId() const { NEXUS_OBJ_MCALL(NXS_InvalidSchedule, getId); }

std::optional<Property> Schedule::getProperty(nxs_int prop) const {
  NEXUS_OBJ_MCALL(std::nullopt, getProperty, prop);
}

Command Schedule::createCommand(Kernel kern) { NEXUS_OBJ_MCALL(Command(), createCommand, kern); }

Command Schedule::createSignalCommand(nxs_int signal_value) { NEXUS_OBJ_MCALL(Command(), createSignalCommand, signal_value); }

Command Schedule::createSignalCommand(Event event, nxs_int signal_value) { NEXUS_OBJ_MCALL(Command(), createSignalCommand, event, signal_value); }

Command Schedule::createWaitCommand(Event event, nxs_int wait_value) { NEXUS_OBJ_MCALL(Command(), createWaitCommand, event, wait_value); }

nxs_status Schedule::run(Stream stream, nxs_bool blocking) { NEXUS_OBJ_MCALL(NXS_InvalidSchedule, run, stream, blocking); }
