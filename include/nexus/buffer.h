#ifndef NEXUS_BUFFER_H
#define NEXUS_BUFFER_H

#include <nexus-api.h>
#include <nexus/object.h>

#include <list>

namespace nexus {
class Device;

namespace detail {
class BufferImpl;
}  // namespace detail

// System class
class Buffer : public Object<detail::BufferImpl> {
 public:
  Buffer(detail::Impl base, size_t _sz, const void *_hostData = nullptr);
  Buffer(detail::Impl base, nxs_int devId, size_t _sz,
         const void *_deviceData = nullptr);
  using Object::Object;

  nxs_int getDeviceId() const;

  std::optional<Property> getProperty(nxs_int prop) const override;

  size_t getSize() const;
  const char *getData() const;
  nxs_data_type getDataType() const;
  size_t getNumElements() const;
  size_t getElementSize() const;

  Buffer getLocal() const;

  nxs_status copy(void *_hostBuf, nxs_uint direction = NXS_BufferDeviceToHost);
};

typedef Objects<Buffer> Buffers;

}  // namespace nexus

#endif  // NEXUS_BUFFER_H