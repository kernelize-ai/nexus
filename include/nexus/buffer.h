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
  Layout(nxs_ulong _size = 0, nxs_uint _data_type = NXS_DataType_Undefined)
    : layout{(nxs_uint)_data_type, (nxs_uint)(_size == 0 ? 0 : 1), {_size}, {0}} {}

  Layout(nxs_ulong *_dims, nxs_uint _dims_count,
         nxs_uint _data_type = NXS_DataType_Undefined);
  
  template <typename T>
  Layout(const std::vector<T> &_dims,
         nxs_uint _data_type = NXS_DataType_Undefined)
      : layout{} {
    layout.data_type = (nxs_uint)_data_type;
    layout.rank = _dims.size();
    for (nxs_uint i = 0; i < layout.rank; i++) {
      layout.dim[i] = _dims[i];
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
  // nxs_ulong getStride(nxs_uint _idx) const;
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

  nxs_ulong getSizeBytes() const;
  const Layout &getLayout() const;
  const char *getData() const;

  Buffer getLocal() const;

  nxs_status copy(void *_hostBuf, nxs_uint direction = NXS_BufferDeviceToHost);
  nxs_status fill(void *value, nxs_uint size_bytes);
};

typedef Objects<Buffer> Buffers;

}  // namespace nexus

#endif  // NEXUS_BUFFER_H