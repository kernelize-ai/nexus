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

  nxs_int getId() const override;

  // Get Device Property Value
  std::optional<Property> getProperty(nxs_int prop) const override;

  Properties getInfo() const;

  // Runtime functions
  Librarys getLibraries() const;
  Schedules getSchedules() const;
  Streams getStreams() const;
  Events getEvents() const;

  Stream createStream();
  Schedule createSchedule();
  Event createEvent(nxs_event_type event_type = NXS_EventType_Shared);

  Library createLibrary(void *libraryData, size_t librarySize);
  Library createLibrary(const std::string &libraryPath);

  Buffer createBuffer(size_t size, const void *data = nullptr,
                      bool on_device = false);
  Buffer copyBuffer(Buffer buf);
  Buffers getBuffers() const;
};

typedef Objects<Device> Devices;

}  // namespace nexus

#endif  // NEXUS_DEVICE_H
