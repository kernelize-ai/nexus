
#include <cstring>

#include <nexus/buffer.h>
#include <nexus/system.h>
#include <nexus/log.h>

#include "_buffer_impl.h"
#include "_runtime_impl.h"
#include "_system_impl.h"

#define NEXUS_LOG_MODULE "buffer"

using namespace nexus;

detail::BufferImpl::BufferImpl(detail::Impl base, size_t _sz, void *_hostData)
: Impl(base), deviceId(NXS_InvalidDevice), size(_sz), data(_hostData) {

}

detail::BufferImpl::BufferImpl(detail::Impl base, nxs_int _devId, size_t _sz, void *_hostData)
: Impl(base), deviceId(_devId), size(_sz), data(_hostData) {

}

detail::BufferImpl::~BufferImpl() {
  release();
}

void detail::BufferImpl::release() {

}

std::optional<Property> detail::BufferImpl::getProperty(nxs_int prop) const {

  return std::nullopt;
}

Buffer detail::BufferImpl::getLocal() const {
  if (!nxs_valid_id(getDeviceId()))
    return *this;
  void *lbuf = malloc(size);
  auto *rt = getParentOfType<RuntimeImpl>();
  if (nxs_success(rt->runAPIFunction<NF_nxsCopyBuffer>(getId(), lbuf))) {
    auto *sys = getParentOfType<detail::SystemImpl>();
    return sys->createBuffer(size, lbuf);
  }
  return Buffer();
}

nxs_status detail::BufferImpl::copyData(void *_hostBuf) const {
  if (nxs_valid_id(getDeviceId())) {
    auto *rt = getParentOfType<RuntimeImpl>();
    return (nxs_status)rt->runAPIFunction<NF_nxsCopyBuffer>(getId(), _hostBuf);
  }
  memcpy(_hostBuf, getHostData(), getSize());
  return NXS_Success;
}


///////////////////////////////////////////////////////////////////////////////
Buffer::Buffer(detail::Impl base, size_t _sz, void *_hostData)
  : Object(base, _sz, _hostData) {}

Buffer::Buffer(detail::Impl base, nxs_int _devId, size_t _sz, void *_hostData)
  : Object(base, _devId, _sz, _hostData) {}

void Buffer::release() const {
  get()->release();
}

nxs_int Buffer::getId() const {
  return get()->getId();
}

nxs_int Buffer::getDeviceId() const {
  return get()->getDeviceId();
}

std::optional<Property> Buffer::getProperty(nxs_int prop) const {
  return get()->getProperty(prop);
}

size_t Buffer::getSize() const {
  return get()->getSize();
}
void *Buffer::getHostData() const {
  return get()->getHostData();
}

Buffer Buffer::getLocal() const {
  return get()->getLocal();
}

nxs_status Buffer::copy(void *_hostBuf) {
  return get()->copyData(_hostBuf);
}
