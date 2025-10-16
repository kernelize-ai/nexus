#include <nexus/command.h>
#include <nexus/event.h>
#include <nexus/kernel.h>
#include <nexus/log.h>
#include <nexus/schedule.h>
#include <nexus/stream.h>

#include "_schedule_impl.h"

#define NEXUS_LOG_MODULE "schedule"

using namespace nexus;
using namespace nexus::detail;

/// @brief Construct a Platform for the current system
ScheduleImpl::ScheduleImpl(detail::Impl base) : detail::Impl(base) {
  NEXUS_LOG(NXS_LOG_NOTE, "  Schedule: ", getId());
}

ScheduleImpl::~ScheduleImpl() {
  NEXUS_LOG(NXS_LOG_NOTE, "  ~Schedule: ", getId());
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

Command ScheduleImpl::createCommand(Kernel kern, nxs_uint settings) {
  auto *rt = getParentOfType<RuntimeImpl>();
  nxs_int cid =
      rt->runAPIFunction<NF_nxsCreateCommand>(getId(), kern.getId(), settings);
  Command cmd(detail::Impl(this, cid, settings), kern);
  commands.add(cmd);
  return cmd;
}

Command ScheduleImpl::createSignalCommand(nxs_int signal_value,
                                          nxs_uint settings) {
  auto *dev = getParentOfType<DeviceImpl>();
  Event event = dev->createEvent();
  auto *rt = getParentOfType<RuntimeImpl>();
  nxs_int cid = rt->runAPIFunction<NF_nxsCreateSignalCommand>(
      getId(), event.getId(), signal_value, settings);
  Command cmd(detail::Impl(this, cid, settings), event);
  commands.add(cmd);
  return cmd;
}

Command ScheduleImpl::createSignalCommand(Event event, nxs_int signal_value,
                                          nxs_uint settings) {
  if (!event) {
    auto *dev = getParentOfType<DeviceImpl>();
    event = dev->createEvent();
  }
  auto *rt = getParentOfType<RuntimeImpl>();
  nxs_int cid = rt->runAPIFunction<NF_nxsCreateSignalCommand>(
      getId(), event.getId(), signal_value, settings);
  Command cmd(detail::Impl(this, cid, settings), event);
  commands.add(cmd);
  return cmd;
}

Command ScheduleImpl::createWaitCommand(Event event, nxs_int wait_value,
                                        nxs_uint settings) {
  auto *rt = getParentOfType<RuntimeImpl>();
  nxs_int cid = rt->runAPIFunction<NF_nxsCreateWaitCommand>(
      getId(), event.getId(), wait_value, settings);
  Command cmd(detail::Impl(this, cid, settings), event);
  commands.add(cmd);
  return cmd;
}

nxs_status ScheduleImpl::run(Stream stream, nxs_uint settings) {
  auto *rt = getParentOfType<RuntimeImpl>();
  return (nxs_status)rt->runAPIFunction<NF_nxsRunSchedule>(
      getId(), stream.getId(), settings);
}

///////////////////////////////////////////////////////////////////////////////
Schedule::Schedule(detail::Impl base) : Object(base) {}

std::optional<Property> Schedule::getProperty(nxs_int prop) const {
  NEXUS_OBJ_MCALL(std::nullopt, getProperty, prop);
}

Command Schedule::createCommand(Kernel kern, nxs_uint settings) {
  NEXUS_OBJ_MCALL(Command(), createCommand, kern, settings);
}

Command Schedule::createSignalCommand(nxs_int signal_value, nxs_uint settings) {
  NEXUS_OBJ_MCALL(Command(), createSignalCommand, signal_value, settings);
}

Command Schedule::createSignalCommand(Event event, nxs_int signal_value,
                                      nxs_uint settings) {
  NEXUS_OBJ_MCALL(Command(), createSignalCommand, event, signal_value,
                  settings);
}

Command Schedule::createWaitCommand(Event event, nxs_int wait_value,
                                    nxs_uint settings) {
  NEXUS_OBJ_MCALL(Command(), createWaitCommand, event, wait_value, settings);
}

nxs_status Schedule::run(Stream stream, nxs_uint settings) {
  NEXUS_OBJ_MCALL(NXS_InvalidSchedule, run, stream, settings);
}

Commands Schedule::getCommands() const {
  NEXUS_OBJ_MCALL(Commands(), getCommands);
}