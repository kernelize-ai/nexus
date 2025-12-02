
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
    : Impl(base), deviceId(NXS_InvalidDevice) {
      setData(_sz, _hostData);
    }

detail::BufferImpl::BufferImpl(detail::Impl base, nxs_int _devId, size_t _sz,
                               const char *_hostData)
    : Impl(base), deviceId(_devId) {
      setData(_sz, _hostData);
    }

detail::BufferImpl::~BufferImpl() { release(); }

void detail::BufferImpl::release() {}

std::optional<Property> detail::BufferImpl::getProperty(nxs_int prop) const {
  if (getDeviceId()) {
    auto *rt = getParentOfType<RuntimeImpl>();
    return rt->getAPIProperty<NF_nxsGetBufferProperty>(prop, getId());
  }
  switch (prop) {
    case NP_Type: return Property("buffer");
    case NP_Size: return Property((nxs_long)data->size());
  }
  return std::nullopt;
}

void detail::BufferImpl::setData(size_t sz, const char *hostData) {
  // TODO: don't copy
  if (hostData != nullptr) {
    data = std::make_shared<std::vector<char>>();
    data->reserve(sz);
    data->insert(data->end(), &hostData[0], &hostData[sz]);
  } else {
    data = std::make_shared<std::vector<char>>(sz);
  }
}

Buffer detail::BufferImpl::getLocal() const {
  if (data) {
    void *lbuf = data->data();
    auto *rt = getParentOfType<RuntimeImpl>();
    if (nxs_success(rt->runAPIFunction<NF_nxsCopyBuffer>(getId(), lbuf, 0))) {
      auto *sys = getParentOfType<detail::SystemImpl>();
      return sys->createBuffer(data->size(), data->data());
    }
  }
  return Buffer();
}

nxs_status detail::BufferImpl::copyData(void *_hostBuf) const {
  if (nxs_valid_id(getDeviceId())) {
    auto *rt = getParentOfType<RuntimeImpl>();
    return (nxs_status)rt->runAPIFunction<NF_nxsCopyBuffer>(getId(), _hostBuf,
                                                            0);
  }
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
