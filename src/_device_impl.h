#ifndef _NEXUS_DEVICE_IMPL_H
#define _NEXUS_DEVICE_IMPL_H

#include <nexus/buffer.h>
#include <nexus/library.h>
#include <nexus/properties.h>
#include <nexus/runtime.h>
#include <nexus/schedule.h>

#include "_runtime_impl.h"

namespace nexus {
namespace detail {

/// @class DesignImpl
class DeviceImpl : public Impl {
  Properties deviceInfo;
  Buffers buffers;
  Librarys libraries;
  Schedules schedules;
  Streams streams;
  Events events;
 public:
  DeviceImpl(Impl base);
  virtual ~DeviceImpl();

  void release();

  RuntimeImpl *getParent() const { return Impl::getParent<RuntimeImpl>(); }

  // Get Runtime Property Value
  std::optional<Property> getProperty(nxs_int prop) const;

  Properties getInfo() const { return deviceInfo; }

  // Runtime functions
  Librarys getLibraries() const { return libraries; }
  Schedules getSchedules() const { return schedules; }
  Streams getStreams() const { return streams; }
  Buffers getBuffers() const { return buffers; }
  Events getEvents() const { return events; }
  Stream createStream();
  Schedule createSchedule();

  Event createEvent(nxs_event_type event_type = NXS_EventType_Shared);
  Library createLibrary(const std::string &path);
  Library createLibrary(void *libraryData, size_t size);

  Buffer createBuffer(size_t size, const char *data = nullptr);
  Buffer copyBuffer(Buffer buf);
};

}  // namespace detail
}  // namespace nexus

#endif  // _NEXUS_DESIGN_IMPL_H