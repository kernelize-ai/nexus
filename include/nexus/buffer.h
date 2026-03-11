#ifndef NEXUS_BUFFER_H
#define NEXUS_BUFFER_H

#include <nexus-api.h>
#include <nexus/object.h>

#include <vector>

namespace nexus {
class Device;

namespace detail {
class BufferImpl;
}  // namespace detail

/// @brief Layout of a buffer, including shape and data type.
class Layout {
 public:
  /// Construct a 1D layout from element count (or rank-0 if size is 0).
  Layout(nxs_ulong _size = 0, nxs_uint _data_type = NXS_DataType_Undefined)
    : layout{_data_type, (nxs_uint)(_size == 0 ? 0 : 1), {_size}, {1}} {}

  /// Construct a layout from explicit dimensions.
  Layout(nxs_ulong *_dims, nxs_uint _dims_count,
         nxs_uint _data_type = NXS_DataType_Undefined);
  
  /// Construct from shape and optional strides (row-major strides are inferred).
  template <typename T>
  Layout(const std::vector<T> &_dims, const std::vector<T> &_strides = {},
         nxs_uint _data_type = NXS_DataType_Undefined)
      : layout{} {
    layout.data_type = (nxs_uint)_data_type;
    layout.rank = _dims.size();
    for (nxs_uint i = 0; i < layout.rank; i++) {
      layout.dim[i] = _dims[i];
      if (i < _strides.size()) {
        layout.stride[i] = _strides[i];
      } else if (i == 0) {
        layout.stride[i] = 1;
      } else {
        layout.stride[i] = layout.dim[i-1] * layout.stride[i-1];
      }
    }
  }

  Layout(const nxs_buffer_layout &_layout) : layout(_layout) {}
  
  operator bool() const { return layout.rank != 0; }

  nxs_uint getRank() const { return layout.rank; }
  nxs_uint getElementSizeBits() const { return nxsGetDataTypeSizeBits(layout.data_type); }
  nxs_ulong getNumElements() const { return nxsGetNumElements(layout); }
  nxs_ulong getDim(nxs_uint _idx) const {
    if (_idx < layout.rank) return layout.dim[_idx];
    return 0;
  }
  nxs_data_type getDataType() const { return nxsGetDataType(layout.data_type); }
  nxs_uint getDataTypeFlags() const { return nxsGetDataTypeFlags(layout.data_type); }
  void setDataType(nxs_uint _data_type) { layout.data_type = _data_type; }
  nxs_ulong getStride(nxs_uint _idx) const {
    if (_idx < layout.rank) return layout.stride[_idx];
    return 0;
  }
  const nxs_buffer_layout &get() const { return layout; }
 private:
  nxs_buffer_layout layout;
};

// System class
class Buffer : public Object<detail::BufferImpl> {
 public:
  Buffer(detail::Impl base, const Layout &layout, const void *_hostData = nullptr);
  using Object::Object;

  std::optional<Property> getProperty(nxs_int prop) const override;

  /// Byte size of the underlying allocation.
  nxs_ulong getSizeBytes() const;
  /// Shape/dtype metadata for this buffer.
  const Layout &getLayout() const;
  /// Raw pointer to the buffer's device-visible storage, if available.
  const char *getDataPtr() const;

  /// Copy buffer contents to/from host memory depending on direction.
  nxs_status copy(void *_hostBuf, nxs_uint direction = NXS_BufferDeviceToHost);
  /// Fill the buffer with a scalar pattern described by `value` bytes.
  nxs_status fill(void *value, nxs_uint size_bytes);
};

typedef Objects<Buffer> Buffers;

}  // namespace nexus

#endif  // NEXUS_BUFFER_H