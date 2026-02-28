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

/// @brief Shape of a buffer
class Shape {
 public:
  Shape(nxs_ulong _size=0) : shape{{_size}, (nxs_uint)(_size == 0 ? 0 : 1)} {}
  Shape(nxs_ulong * _dims, nxs_uint _dims_count);
  Shape(nxs_shape _shape) : shape(_shape) {}
  nxs_uint getRank() const { return shape.rank; }
  nxs_ulong getNumElements() const { return nxsGetNumElements(shape); }
  nxs_ulong getDim(nxs_uint _idx) const {
    if (_idx < shape.rank) return shape.dims[_idx];
    return 0;
  }
  //nxs_ulong getStride(nxs_uint _idx) const;
  nxs_shape get() const { return shape; }
 private:
  nxs_shape shape;
};

// System class
class Buffer : public Object<detail::BufferImpl> {
 public:
  Buffer(detail::Impl base, const Shape &shape, const void *_hostData = nullptr);
  using Object::Object;

  std::optional<Property> getProperty(nxs_int prop) const override;

  nxs_ulong getSizeBytes() const;
  Shape getShape() const;
  const char *getData() const;
  nxs_data_type getDataType() const;
  nxs_uint getDataTypeFlags() const;
  nxs_ulong getNumElements() const;
  nxs_uint getElementSizeBits() const;

  Buffer getLocal() const;

  nxs_status copy(void *_hostBuf, nxs_uint direction = NXS_BufferDeviceToHost);
  nxs_status fill(void *value, nxs_uint size_bytes);
};

typedef Objects<Buffer> Buffers;

}  // namespace nexus

#endif  // NEXUS_BUFFER_H