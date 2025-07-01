#include <nexus/buffer.h>
#include <nexus/device_db.h>
#include <nexus/log.h>
#include <nexus/runtime.h>

#include <filesystem>

#include "_buffer_impl.h"
#include "_device_impl.h"
#include "_runtime_impl.h"

#define NEXUS_LOG_MODULE "device"

using namespace nexus;

#define APICALL(FUNC, ...)                                                   \
  nxs_int apiResult = getParent()->runAPIFunction<NF_##FUNC>(__VA_ARGS__)

detail::DeviceImpl::DeviceImpl(detail::Impl base) : detail::Impl(base) {
  auto id = getId();
  auto vendor = getProperty(NP_Vendor);
  auto type = getProperty(NP_Type);
  auto arch = getProperty(NP_Architecture);
  if (!vendor || !type || !arch) return;

  auto devTag = vendor->getValue<NP_Vendor>() + "-" +
                type->getValue<NP_Type>() + "-" +
                arch->getValue<NP_Architecture>();
  NEXUS_LOG(NEXUS_STATUS_NOTE, "    DeviceTag: " << devTag);
  if (auto props = nexus::lookupDeviceInfo(devTag))
    deviceInfo = props;
  else  // load defaults
    NEXUS_LOG(NEXUS_STATUS_ERR, "    Device Properties not found");
}

detail::DeviceImpl::~DeviceImpl() {
  NEXUS_LOG(NEXUS_STATUS_NOTE, "    ~Device: " << getId());
  release();
}

void detail::DeviceImpl::release() {
  NEXUS_LOG(NEXUS_STATUS_NOTE, "    release: " << getId());
  // Tear down order is important for backend plugins
  buffers.clear();

  schedules.clear();
  streams.clear();
  libraries.clear();
}

std::optional<Property> detail::DeviceImpl::getProperty(nxs_int prop) const {
  NEXUS_LOG(NEXUS_STATUS_NOTE, "Device.getProperty: " << prop);
  auto *runtime = getParent();
  if (auto fn = runtime->getFunction<NF_nxsGetDeviceProperty>()) {
    auto npt_prop = nxs_property_type_map[prop];
    if (npt_prop == NPT_INT) {
      nxs_long val = 0;
      size_t size = sizeof(val);
      if (nxs_success((*fn)(getId(), prop, &val, &size))) return Property(val);
    } else if (npt_prop == NPT_FLT) {
      nxs_double val = 0.;
      size_t size = sizeof(val);
      if (nxs_success((*fn)(getId(), prop, &val, &size))) return Property(val);
    } else if (npt_prop == NPT_STR) {
      size_t size = 256;
      char name[size];
      name[0] = '\0';
      if (nxs_success((*fn)(getId(), prop, &name, &size)))
        return std::string(name);
    } else {
      NEXUS_LOG(NEXUS_STATUS_ERR,
                "Device.getProperty: Unknown property type for - "
                    << nxsGetPropName(prop));
      // assert(0);
    }
  }
  return std::nullopt;
}

// Runtime functions
Library detail::DeviceImpl::createLibrary(void *data, size_t size) {
  NEXUS_LOG(NEXUS_STATUS_NOTE, "  createLibrary - Size: " << size);
  APICALL(nxsCreateLibrary, getId(), data, size);
  Library lib(detail::Impl(this, apiResult));
  libraries.add(lib);
  return lib;
}

Library detail::DeviceImpl::createLibrary(const std::string &path) {
  NEXUS_LOG(NEXUS_STATUS_NOTE, "  createLibrary - Path: " << path);
  APICALL(nxsCreateLibraryFromFile, getId(), path.c_str());
  Library lib(detail::Impl(this, apiResult));
  libraries.add(lib);
  return lib;
}

Schedule detail::DeviceImpl::createSchedule() {
  NEXUS_LOG(NEXUS_STATUS_NOTE, "  createSchedule");
  APICALL(nxsCreateSchedule, getId(), 0);
  Schedule sched(detail::Impl(this, apiResult));
  schedules.add(sched);
  return sched;
}

Stream detail::DeviceImpl::createStream() {
  NEXUS_LOG(NEXUS_STATUS_NOTE, "  createStream");
  APICALL(nxsCreateStream, getId(), 0);
  Stream stream(detail::Impl(this, apiResult));
  streams.add(stream);
  return stream;
}

Buffer detail::DeviceImpl::createBuffer(size_t size, const char *data) {
  NEXUS_LOG(NEXUS_STATUS_NOTE, "  createBuffer");
  APICALL(nxsCreateBuffer, getId(), size, 0, (void *)data);
  Buffer nbuf(Impl(this, apiResult), getId(), size);
  buffers.add(nbuf);
  return nbuf;
}

Buffer detail::DeviceImpl::copyBuffer(Buffer buf) {
  NEXUS_LOG(NEXUS_STATUS_NOTE, "  copyBuffer");
  APICALL(nxsCreateBuffer, getId(), buf.getSize(), 0, (void *)buf.getData());
  Buffer nbuf(Impl(this, apiResult), getId(), buf.getSize());
  buffers.add(nbuf);
  return nbuf;
}

///////////////////////////////////////////////////////////////////////////////
/// Object wrapper - Device
///////////////////////////////////////////////////////////////////////////////
Device::Device(detail::Impl base) : Object(base) {}

nxs_int Device::getId() const { NEXUS_OBJ_MCALL(NXS_InvalidDevice, getId); }

// Get Device Property Value
std::optional<Property> Device::getProperty(nxs_int prop) const {
  NEXUS_OBJ_MCALL(std::nullopt, getProperty, prop);
}

Properties Device::getInfo() const { NEXUS_OBJ_MCALL(Properties(), getInfo); }

// Runtime functions
Librarys Device::getLibraries() const { NEXUS_OBJ_MCALL(Librarys(), getLibraries); }

Schedules Device::getSchedules() const { NEXUS_OBJ_MCALL(Schedules(), getSchedules); }

Streams Device::getStreams() const { NEXUS_OBJ_MCALL(Streams(), getStreams); }

Buffers Device::getBuffers() const { NEXUS_OBJ_MCALL(Buffers(), getBuffers); }

Stream Device::createStream() { NEXUS_OBJ_MCALL(Stream(), createStream); }

Schedule Device::createSchedule() { NEXUS_OBJ_MCALL(Schedule(), createSchedule); }

Buffer Device::createBuffer(size_t size, const void *data) {
  NEXUS_OBJ_MCALL(Buffer(), createBuffer, size, (const char *)data);
}

Buffer Device::copyBuffer(Buffer buf) { NEXUS_OBJ_MCALL(Buffer(), copyBuffer, buf); }

Library Device::createLibrary(void *libraryData, size_t librarySize) {
  NEXUS_OBJ_MCALL(Library(), createLibrary, libraryData, librarySize);
}

Library Device::createLibrary(const std::string &libraryPath) {
  NEXUS_OBJ_MCALL(Library(), createLibrary, libraryPath);
}
