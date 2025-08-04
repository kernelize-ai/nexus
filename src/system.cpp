
#include <nexus/log.h>
#include <nexus/system.h>
#include <nexus/utility.h>

#include "_system_impl.h"

using namespace nexus;
using namespace nexus::detail;

#define NEXUS_LOG_MODULE "system"

/// @brief Construct a Platform for the current system
SystemImpl::SystemImpl(int) {
  NEXUS_LOG(NEXUS_STATUS_NOTE, "CTOR");
  iterateEnvPaths("NEXUS_RUNTIME_PATH", "./runtime_libs",
                  [&](const std::string &path, const std::string &name) {
                    Runtime rt(detail::Impl(this, runtimes.size()), path);
                    runtimes.add(rt);
                    runtimeMap[rt.getProp<std::string>(NP_Name)] = rt;
                  });
}

SystemImpl::~SystemImpl() {
  NEXUS_LOG(NEXUS_STATUS_NOTE, "DTOR");
  // for (auto rt : runtimes)
  //   rt.release();
  // for (auto buf : buffers)
  //   buf.release();
}

std::optional<Property> SystemImpl::getProperty(nxs_int prop) const {
  return std::nullopt;
}

Buffer SystemImpl::createBuffer(size_t sz, const void *hostData,
                                nxs_uint settings) {
  NEXUS_LOG(NEXUS_STATUS_NOTE, "createBuffer " << sz);
  nxs_uint id = buffers.size();
  Buffer buf(detail::Impl(this, id, settings), sz, hostData);
  buffers.add(buf);
  return buf;
}

Buffer SystemImpl::copyBuffer(Buffer buf, Device dev, nxs_uint settings) {
  NEXUS_LOG(NEXUS_STATUS_NOTE, "copyBuffer " << buf.getSize());
  Buffer nbuf = dev.copyBuffer(buf, settings);
  return nbuf;
}

///////////////////////////////////////////////////////////////////////////////
/// @param
System::System(int i) : Object(i) {}

std::optional<Property> System::getProperty(nxs_int prop) const {
  NEXUS_OBJ_MCALL(std::nullopt, getProperty, prop);
}

Buffers System::getBuffers() const { NEXUS_OBJ_MCALL(Buffers(), getBuffers); }

Runtimes System::getRuntimes() const { NEXUS_OBJ_MCALL(Runtimes(), getRuntimes); }

Runtime System::getRuntime(int idx) const { NEXUS_OBJ_MCALL(Runtime(), getRuntime, idx); }
Runtime System::getRuntime(const std::string &name) { NEXUS_OBJ_MCALL(Runtime(), getRuntime, name); }

Buffer System::createBuffer(size_t sz, const void *hostData,
                            nxs_uint settings) {
  NEXUS_OBJ_MCALL(Buffer(), createBuffer, sz, hostData, settings);
}

Buffer System::copyBuffer(Buffer buf, Device dev, nxs_uint settings) {
  NEXUS_OBJ_MCALL(Buffer(), copyBuffer, buf, dev, settings);
}

/// @brief Get the System Platform
/// @return
nexus::System nexus::getSystem() {
  static System s_system(0);
  return s_system;
}
