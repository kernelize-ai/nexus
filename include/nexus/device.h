#ifndef NEXUS_DEVICE_H
#define NEXUS_DEVICE_H

#include <nexus-api.h>
#include <nexus/buffer.h>
#include <nexus/event.h>
#include <nexus/library.h>
#include <nexus/properties.h>
#include <nexus/schedule.h>
#include <nexus/stream.h>

#include <optional>
#include <string>

namespace nexus {

namespace detail {
class DeviceImpl;
}  // namespace detail

// Device class
class Device : public Object<detail::DeviceImpl> {
 public:
  Device(detail::Impl base);
  using Object::Object;

  // Get Device Property Value
  std::optional<Property> getProperty(nxs_int prop) const override;

  Properties getInfo() const;

  // Runtime functions
  Librarys getLibraries() const;
  Schedules getSchedules() const;
  Streams getStreams() const;
  Events getEvents() const;
  Buffers getBuffers() const;

  Stream createStream(nxs_uint settings = 0);
  Schedule createSchedule(nxs_uint settings = 0);
  Event createEvent(nxs_event_type event_type = NXS_EventType_Shared,
                    nxs_uint settings = 0);

  Library createLibrary(void *libraryData, size_t librarySize,
                        nxs_uint settings = 0);
  Library createLibrary(const std::string &libraryPath, nxs_uint settings = 0);

  Buffer createBuffer(size_t size, const void *data = nullptr,
                      nxs_uint settings = 0);
  Buffer copyBuffer(Buffer buf, nxs_uint settings = 0);
};

typedef Objects<Device> Devices;

}  // namespace nexus

#endif  // NEXUS_DEVICE_H
