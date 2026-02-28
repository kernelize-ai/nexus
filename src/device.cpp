#include <nexus/buffer.h>
#include <nexus/device_db.h>
#include <nexus/log.h>
#include <nexus/runtime.h>
#include <nexus/utility.h>

#include <filesystem>

#include "_buffer_impl.h"
#include "_device_impl.h"
#include "_info_impl.h"
#include "_runtime_impl.h"

#define NEXUS_LOG_MODULE "device"

using namespace nexus;

#define APICALL(FUNC, ...)                                                   \
  nxs_int apiResult = getParent()->runAPIFunction<NF_##FUNC>(__VA_ARGS__)

detail::DeviceImpl::DeviceImpl(detail::Impl base) : detail::Impl(base) {
  auto vendor = getProperty(NP_Vendor);
  auto type = getProperty(NP_Type);
  auto arch = getProperty(NP_Architecture);
  if (!vendor || !type || !arch) return;

  auto devTag = vendor->getValue<NP_Vendor>() + "-" +
                type->getValue<NP_Type>() + "-" +
                arch->getValue<NP_Architecture>();
  NEXUS_LOG(NXS_LOG_NOTE, "    DeviceTag: ", devTag);
  if (auto info = nexus::lookupDeviceInfo(devTag))
    deviceInfo = info;
  else  // load defaults
    NEXUS_LOG(NXS_LOG_ERROR, "    Device Properties not found");
}

detail::DeviceImpl::~DeviceImpl() {
  NEXUS_LOG(NXS_LOG_NOTE, "    ~Device: ", getId());
  release();
}

void detail::DeviceImpl::release() {
  NEXUS_LOG(NXS_LOG_NOTE, "    release: ", getId());
  // Tear down order is important for backend plugins
  buffers.clear();

  schedules.clear();
  streams.clear();
  libraries.clear();
}

std::optional<Property> detail::DeviceImpl::getProperty(nxs_int prop) const {
  auto *rt = getParentOfType<RuntimeImpl>();
  return rt->getAPIProperty<NF_nxsGetDeviceProperty>(prop, getId());
}

// Runtime functions
// Private helper function to find a library in a catalog
struct LibraryInfo {
  std::string arch;
  std::string binaryData;
  nxs_long size;
  Info libraryNode;
};

static void findDeviceBinary(LibraryInfo &info, Info catalogInfo,
                             const std::string &libraryName,
                             const std::string &arch) {
  if (auto libs = catalogInfo.getNode({"Libraries"})) {
    for (auto lib : *libs) {
      try {
        auto name = lib.at("Name").get<std::string_view>();
        if (name == libraryName) {
          for (auto &narch : lib.at("Architectures")) {
            // TODO: check compatibility
            auto narchName = narch.at("Name").get<std::string_view>();
            if (narchName == arch) {
              // TODO: check
              auto binaryData = narch.at("BinaryData").get<std::string_view>();
              auto size = narch.at("FileSize").get<nxs_long>();
              info.arch = narchName.data();
              info.binaryData = binaryData.data();
              info.size = size;
              Info::Node node(lib);
              info.libraryNode = Info(node);
              break;
            }
          }
        }
      } catch (...) {
        NEXUS_LOG(NXS_LOG_ERROR, "  binary not found");
      }
    }
  }
}

Library detail::DeviceImpl::loadLibrary(Info catalog,
                                        const std::string &libraryName) {
  NEXUS_LOG(NXS_LOG_NOTE, "  loadLibrary");
  auto arch = getProperty(NP_Architecture)->getValue<std::string>();
  LibraryInfo libInfo;
  findDeviceBinary(libInfo, catalog, libraryName, arch);
  if (libInfo.arch.empty()) {
    NEXUS_LOG(NXS_LOG_ERROR, "  library not found");
    return Library();
  }
  // std::vector<uint8_t> data = base64Decode(libInfo.binaryData, libInfo.size);
  APICALL(nxsCreateLibrary, getId(), (void *)libInfo.binaryData.data(),
          libInfo.binaryData.size(), 0);
  Library lib(detail::Impl(this, apiResult, 0), libInfo.libraryNode);
  libraries.add(lib);
  return lib;
}

Library detail::DeviceImpl::createLibrary(void *data, size_t size,
                                          nxs_uint settings) {
  NEXUS_LOG(NXS_LOG_NOTE, "  createLibrary - Size: ", size);
  APICALL(nxsCreateLibrary, getId(), data, size, settings);
  Library lib(detail::Impl(this, apiResult, settings));
  libraries.add(lib);
  return lib;
}

Library detail::DeviceImpl::createLibrary(const std::string &path,
                                          nxs_uint settings) {
  NEXUS_LOG(NXS_LOG_NOTE, "  createLibrary - Path: ", path);
  APICALL(nxsCreateLibraryFromFile, getId(), path.c_str(), settings);
  Library lib(detail::Impl(this, apiResult, settings));
  libraries.add(lib);
  return lib;
}

Schedule detail::DeviceImpl::createSchedule(nxs_uint settings) {
  NEXUS_LOG(NXS_LOG_NOTE, "  createSchedule");
  APICALL(nxsCreateSchedule, getId(), settings);
  Schedule sched(detail::Impl(this, apiResult, settings));
  schedules.add(sched);
  return sched;
}

Stream detail::DeviceImpl::createStream(nxs_uint settings) {
  NEXUS_LOG(NXS_LOG_NOTE, "  createStream");
  APICALL(nxsCreateStream, getId(), 0);
  Stream stream(detail::Impl(this, apiResult, settings));
  streams.add(stream);
  return stream;
}

Event detail::DeviceImpl::createEvent(nxs_event_type event_type,
                                      nxs_uint settings) {
  NEXUS_LOG(NXS_LOG_NOTE, "  createEvent");
  APICALL(nxsCreateEvent, getId(), event_type, settings);
  Event event(detail::Impl(this, apiResult, settings));
  events.add(event);
  return event;
}

Buffer detail::DeviceImpl::createBuffer(const Shape &shape, const void *data,
                                        nxs_uint settings) {
  NEXUS_LOG(NXS_LOG_NOTE, "  createBuffer");
  APICALL(nxsCreateBuffer, getId(), shape.get(), (void *)data, settings);
  Buffer nbuf(Impl(this, apiResult, settings), shape, data);
  buffers.add(nbuf);
  return nbuf;
}

Buffer detail::DeviceImpl::copyBuffer(Buffer buf, nxs_uint settings) {
  NEXUS_LOG(NXS_LOG_NOTE, "  copyBuffer");
  settings |= buf.getSettings() & ~NXS_BufferSettings_OnDevice;
  APICALL(nxsCreateBuffer, getId(), buf.getShape().get(), (void *)buf.getData(),
          settings);
  Buffer nbuf(Impl(this, apiResult, settings), buf.getShape(), buf.getData());
  buffers.add(nbuf);
  return nbuf;
}

///////////////////////////////////////////////////////////////////////////////
/// Object wrapper - Device
///////////////////////////////////////////////////////////////////////////////
Device::Device(detail::Impl base) : Object(base) {}

// Get Device Property Value
std::optional<Property> Device::getProperty(nxs_int prop) const {
  NEXUS_OBJ_MCALL(std::nullopt, getProperty, prop);
}

Info Device::getInfo() const { NEXUS_OBJ_MCALL(Info(), getInfo); }

// Runtime functions
Librarys Device::getLibraries() const { NEXUS_OBJ_MCALL(Librarys(), getLibraries); }

Schedules Device::getSchedules() const { NEXUS_OBJ_MCALL(Schedules(), getSchedules); }

Streams Device::getStreams() const { NEXUS_OBJ_MCALL(Streams(), getStreams); }

Buffers Device::getBuffers() const { NEXUS_OBJ_MCALL(Buffers(), getBuffers); }

Events Device::getEvents() const { NEXUS_OBJ_MCALL(Events(), getEvents); }

Event Device::createEvent(nxs_event_type event_type, nxs_uint settings) {
  NEXUS_OBJ_MCALL(Event(), createEvent, event_type, settings);
}

Stream Device::createStream(nxs_uint settings) {
  NEXUS_OBJ_MCALL(Stream(), createStream, settings);
}

Schedule Device::createSchedule(nxs_uint settings) {
  NEXUS_OBJ_MCALL(Schedule(), createSchedule, settings);
}

Buffer Device::createBuffer(const Shape &shape, const void *data, nxs_uint settings) {
  NEXUS_OBJ_MCALL(Buffer(), createBuffer, shape, data, settings);
}

Buffer Device::copyBuffer(Buffer buf, nxs_uint settings) {
  NEXUS_OBJ_MCALL(Buffer(), copyBuffer, buf, settings);
}

Library Device::loadLibrary(Info catalog, const std::string &libraryName) {
  NEXUS_OBJ_MCALL(Library(), loadLibrary, catalog, libraryName);
}

Library Device::createLibrary(void *libraryData, size_t librarySize,
                              nxs_uint settings) {
  NEXUS_OBJ_MCALL(Library(), createLibrary, libraryData, librarySize, settings);
}

Library Device::createLibrary(const std::string &libraryPath,
                              nxs_uint settings) {
  NEXUS_OBJ_MCALL(Library(), createLibrary, libraryPath, settings);
}
