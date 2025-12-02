
#include <nexus/buffer.h>
#include <nexus/log.h>
#include <nexus/system.h>

#include <cstring>

#include "_buffer_impl.h"
#include "_runtime_impl.h"
#include "_system_impl.h"

#define NEXUS_LOG_MODULE "buffer"

using namespace nexus;

detail::BufferImpl::BufferImpl(detail::Impl base, size_t _sz, const char *_hostData)
    : Impl(base), deviceId(NXS_InvalidDevice), size(0), data(nullptr) {
      setData(_sz, _hostData);
    }

detail::BufferImpl::BufferImpl(detail::Impl base, nxs_int _devId, size_t _sz,
                               const char *_hostData)
    : Impl(base), deviceId(_devId), size(0), data(nullptr) {
      setData(_sz, _hostData);
    }

detail::BufferImpl::~BufferImpl() { release(); }

void detail::BufferImpl::release() {
  if (getSettings() & NXS_BufferSettings_Maintain) {
    delete static_cast<DataBuf*>(data);
  }
  data = nullptr;
  size = 0;
}

void *detail::BufferImpl::getVoidData() const {
  if (getSettings() & NXS_BufferSettings_Maintain) {
    return static_cast<void *>(static_cast<DataBuf*>(data)->data());
  }
  return data;
}

const char *detail::BufferImpl::getData() const {
  return reinterpret_cast<const char *>(getVoidData());
}

std::optional<Property> detail::BufferImpl::getProperty(nxs_int prop) const {
  if (getDeviceId()) {
    auto *rt = getParentOfType<RuntimeImpl>();
    return rt->getAPIProperty<NF_nxsGetBufferProperty>(prop, getId());
  }
  switch (prop) {
    case NP_Type: return Property("buffer");
    case NP_Size: return Property((nxs_long)size);
  }
  return std::nullopt;
}

void detail::BufferImpl::setData(size_t sz, const char *hostData) {
  size = sz;
  if (getSettings() & NXS_BufferSettings_Maintain) {
    NEXUS_LOG(NXS_LOG_NOTE, "setData: maintain: ", sz);
    // SHOULD BE RARE
    if (hostData != nullptr) {
      data = new DataBuf();
      static_cast<DataBuf*>(data)->assign(hostData, hostData + sz);
    } else {
      data = new DataBuf(sz);
    }
  } else {
    NEXUS_LOG(NXS_LOG_NOTE, "setData: not maintain: ", sz);
    data = const_cast<void *>(reinterpret_cast<const void *>(hostData));
  }
}

Buffer detail::BufferImpl::getLocal() {
  NEXUS_LOG(NXS_LOG_NOTE, "getLocal: ", getVoidData());
  void *lbuf = getVoidData();
  if (!lbuf) {
    setSettings(getSettings() | NXS_BufferSettings_Maintain);
    setData(size, nullptr);
    lbuf = getVoidData();
  }
  if (nxs_valid_id(getDeviceId())) {
    auto *rt = getParentOfType<RuntimeImpl>();
    if (nxs_success(rt->runAPIFunction<NF_nxsCopyBuffer>(getId(), lbuf, 0))) {
      auto *sys = getParentOfType<detail::SystemImpl>();
      return sys->createBuffer(size, reinterpret_cast<const char *>(lbuf));
    }
  }
  return Buffer();
}

nxs_status detail::BufferImpl::copyData(void *_hostBuf) const {
  if (nxs_valid_id(getDeviceId())) {
    NEXUS_LOG(NXS_LOG_NOTE, "copyData: from device: ", getSize());
    auto *rt = getParentOfType<RuntimeImpl>();
    return (nxs_status)rt->runAPIFunction<NF_nxsCopyBuffer>(getId(), _hostBuf,
                                                            0);
  }
  NEXUS_LOG(NXS_LOG_NOTE, "copyData: from host: ", getSize());
  memcpy(_hostBuf, getData(), getSize());
  return NXS_Success;
}

///////////////////////////////////////////////////////////////////////////////
Buffer::Buffer(detail::Impl base, size_t _sz, const void *_hostData)
    : Object(base, _sz, (const char *)_hostData) {}

Buffer::Buffer(detail::Impl base, nxs_int _devId, size_t _sz, const void *_hostData)
    : Object(base, _devId, _sz, (const char *)_hostData) {}

nxs_int Buffer::getDeviceId() const { NEXUS_OBJ_MCALL(NXS_InvalidBuffer, getDeviceId); }

std::optional<Property> Buffer::getProperty(nxs_int prop) const {
  NEXUS_OBJ_MCALL(std::nullopt, getProperty, prop);
}

size_t Buffer::getSize() const { NEXUS_OBJ_MCALL(0, getSize); }
const char *Buffer::getData() const { NEXUS_OBJ_MCALL(nullptr, getData); }

Buffer Buffer::getLocal() const {
  if (!nxs_valid_id(getDeviceId())) return *this;
  return get()->getLocal();
}

nxs_status Buffer::copy(void *_hostBuf) { NEXUS_OBJ_MCALL(NXS_InvalidBuffer, copyData, _hostBuf); }
