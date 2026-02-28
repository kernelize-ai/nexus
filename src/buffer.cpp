
#include <nexus/buffer.h>
#include <nexus/log.h>
#include <nexus/system.h>

#include <cstring>

#include "_buffer_impl.h"
#include "_device_impl.h"
#include "_runtime_impl.h"
#include "_system_impl.h"

#define NEXUS_LOG_MODULE "buffer"

using namespace nexus;

detail::BufferImpl::BufferImpl(detail::Impl base, const Shape &shape, const char *_hostData)
    : Impl(base), shape(shape), size_bytes(0), data(nullptr) {
  nxs_ulong size_bytes = getNumElements();
  if (auto element_size_bits = getElementSizeBits()) {
    size_bytes *= element_size_bits;
    size_bytes /= 8;
  }
  setData(size_bytes, _hostData);
}

detail::BufferImpl::~BufferImpl() { release(); }

void detail::BufferImpl::release() {
  if (data != nullptr) {
    if (hasSetting(NXS_BufferSettings_Maintain)) {
      delete static_cast<StorageType *>(data);
    }
    data = nullptr;
  }
  size_bytes = 0;
}

void *detail::BufferImpl::getVoidData() const {
  if (hasSetting(NXS_BufferSettings_Maintain)) {
    return static_cast<void *>(static_cast<StorageType *>(data)->data());
  }
  return data;
}

const char *detail::BufferImpl::getData() const {
  return reinterpret_cast<const char *>(getVoidData());
}

nxs_data_type detail::BufferImpl::getDataType() const {
  return nxsGetDataType(getSettings());
}

nxs_uint detail::BufferImpl::getDataTypeFlags() const {
  return nxsGetDataTypeFlags(getSettings());
}

nxs_ulong detail::BufferImpl::getNumElements() const {
  return shape.getNumElements();
}

nxs_uint detail::BufferImpl::getElementSizeBits() const {
  return nxsGetDataTypeSizeBits(getDataType());
}

std::optional<Property> detail::BufferImpl::getProperty(nxs_int prop) const {
  switch (prop) {
    case NP_Type: return Property("buffer");
    case NP_Size:
      return Property((nxs_long)size_bytes);
    case NP_Shape: {
      PropIntVec dims;
      for (nxs_uint i = 0; i < shape.getRank(); i++) {
        dims.push_back(shape.getDim(i));
      }
      return Property(dims);
    }
  }
  if (getParentOfType<DeviceImpl>()) {
    auto *rt = getParentOfType<RuntimeImpl>();
    return rt->getAPIProperty<NF_nxsGetBufferProperty>(prop, getId());
  }
  return std::nullopt;
}

void detail::BufferImpl::setData(nxs_ulong sz, const char *hostData) {
  if (data != nullptr) {
    release();
  }
  size_bytes = sz;
  if (getSettings() & NXS_BufferSettings_Maintain) {
    NEXUS_LOG(NXS_LOG_NOTE, "setData: maintain: ", sz);
    // SHOULD BE RARE
    if (hostData != nullptr) {
      data = new StorageType();
      static_cast<StorageType *>(data)->assign(hostData, hostData + sz);
    } else {
      data = new StorageType(sz);
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
    setSetting(NXS_BufferSettings_Maintain);
    setData(size_bytes, nullptr);
    lbuf = getVoidData();
  }
  if (getParentOfType<DeviceImpl>()) {
    auto *rt = getParentOfType<RuntimeImpl>();
    if (nxs_success(rt->runAPIFunction<NF_nxsCopyBuffer>(getId(), lbuf, 0))) {
      auto *sys = getParentOfType<detail::SystemImpl>();
      return sys->createBuffer(shape, reinterpret_cast<const char *>(lbuf));
    }
  }
  return *this;
}

nxs_status detail::BufferImpl::copyData(void *_hostBuf, nxs_uint direction) const {
  if (getParentOfType<DeviceImpl>()) {
    NEXUS_LOG(NXS_LOG_NOTE, "copyData: from device: ", getSizeBytes());
    auto *rt = getParentOfType<RuntimeImpl>();
    return (nxs_status)rt->runAPIFunction<NF_nxsCopyBuffer>(getId(), _hostBuf,
                                                            direction);
  }
  NEXUS_LOG(NXS_LOG_NOTE, "copyData: from host: ", getSizeBytes());
  std::memcpy(_hostBuf, getData(), getSizeBytes());
  return NXS_Success;
}

nxs_status detail::BufferImpl::fillData(void *value, nxs_uint size_bytes) const {
  nxs_status return_stat;
  if (getParentOfType<DeviceImpl>()) {
    NEXUS_LOG(NXS_LOG_NOTE, "fillData: on device: ", getSizeBytes());
    auto *rt = getParentOfType<RuntimeImpl>();
    return_stat = (nxs_status)rt->runAPIFunction<NF_nxsFillBuffer>(getId(), value, size_bytes);
  }
  NEXUS_LOG(NXS_LOG_NOTE, "fillData: on host: ", getSizeBytes());
  return return_stat;
}

///////////////////////////////////////////////////////////////////////////////
Buffer::Buffer(detail::Impl base, const Shape &shape, const void *_hostData)
    : Object(base, shape, (const char *)_hostData) {}

std::optional<Property> Buffer::getProperty(nxs_int prop) const {
  NEXUS_OBJ_MCALL(std::nullopt, getProperty, prop);
}

nxs_ulong Buffer::getSizeBytes() const { NEXUS_OBJ_MCALL(0, getSizeBytes); }
Shape Buffer::getShape() const { NEXUS_OBJ_MCALL(Shape(), getShape); }
const char *Buffer::getData() const { NEXUS_OBJ_MCALL(nullptr, getData); }
nxs_data_type Buffer::getDataType() const { NEXUS_OBJ_MCALL(NXS_DataType_Undefined, getDataType); }
nxs_ulong Buffer::getNumElements() const { NEXUS_OBJ_MCALL(0, getNumElements); }
nxs_uint Buffer::getElementSizeBits() const { NEXUS_OBJ_MCALL(0, getElementSizeBits); }

Buffer Buffer::getLocal() const {
  return get()->getLocal();
}

nxs_status Buffer::copy(void *_hostBuf, nxs_uint direction) { NEXUS_OBJ_MCALL(NXS_InvalidBuffer, copyData, _hostBuf, direction); }
nxs_status Buffer::fill(void *value, nxs_uint size_bytes) { NEXUS_OBJ_MCALL(NXS_InvalidBuffer, fillData, value, size_bytes); }

Shape::Shape(nxs_ulong * _dims, nxs_uint _dims_count) {
  shape.rank = _dims_count;
  std::copy(_dims, _dims + _dims_count, shape.dims);
}