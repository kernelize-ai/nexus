#ifndef NEXUS_DEVICE_H
#define NEXUS_DEVICE_H

#include <nexus-api.h>
#include <nexus/buffer.h>
#include <nexus/library.h>
#include <nexus/properties.h>
#include <nexus/schedule.h>

#include <optional>
#include <string>

namespace nexus {

namespace detail {
class RuntimeImpl;  // owner
class DeviceImpl;
}  // namespace detail

// Device class
class Device : public Object<detail::DeviceImpl, detail::RuntimeImpl> {
  friend OwnerTy;

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

  Schedule createSchedule();

  Library createLibrary(void *libraryData, size_t librarySize);
  Library createLibrary(const std::string &libraryPath);

  Buffer createBuffer(size_t _sz, const void *_hostData = nullptr);
  Buffer copyBuffer(Buffer buf);
};

typedef Objects<Device> Devices;

}  // namespace nexus

#endif  // NEXUS_DEVICE_H
