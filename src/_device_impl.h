#ifndef _NEXUS_DEVICE_IMPL_H
#define _NEXUS_DEVICE_IMPL_H

#include <nexus/buffer.h>
#include <nexus/library.h>
#include <nexus/info.h>
#include <nexus/runtime.h>
#include <nexus/schedule.h>

#include "_runtime_impl.h"

namespace nexus {
namespace detail {

/// @class DesignImpl
class DeviceImpl : public Impl {
  Info deviceInfo;
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

  Info getInfo() const { return deviceInfo; }

  // Runtime functions
  Librarys getLibraries() const { return libraries; }
  Schedules getSchedules() const { return schedules; }
  Streams getStreams() const { return streams; }
  Buffers getBuffers() const { return buffers; }
  Events getEvents() const { return events; }

  // Create objects
  Stream createStream(nxs_uint settings = 0);
  Schedule createSchedule(nxs_uint settings = 0);
  Event createEvent(nxs_event_type event_type = NXS_EventType_Shared,
                    nxs_uint settings = 0);
  Library loadLibrary(Info catalog, const std::string &libraryName);
  Library createLibrary(const std::string &path, nxs_uint settings = 0);
  Library createLibrary(void *libraryData, size_t size, nxs_uint settings = 0);

  Buffer createBuffer(const Shape &shape, const void *data = nullptr,
                      nxs_uint settings = 0);
  Buffer copyBuffer(Buffer buf, nxs_uint settings = 0);
  Buffer fillBuffer(void *value, nxs_uint value_size_bytes);
};

}  // namespace detail
}  // namespace nexus

#endif  // _NEXUS_DESIGN_IMPL_H