#ifndef _NEXUS_BUFFER_IMPL_H
#define _NEXUS_BUFFER_IMPL_H

#include <nexus/device.h>

namespace nexus {
namespace detail {
class BufferImpl : public Impl {
 public:
  BufferImpl(Impl base, const Shape &shape, const char *_hostData);

  ~BufferImpl();

  void release();

  std::optional<Property> getProperty(nxs_int prop) const;

  nxs_ulong getSizeBytes() const { return size_bytes; }
  const Shape &getShape() const { return shape; }
  const char *getData() const;
  nxs_data_type getDataType() const;
  nxs_uint getDataTypeFlags() const;
  nxs_ulong getNumElements() const;
  nxs_uint getElementSizeBits() const;

  void setData(nxs_ulong sz, const char *hostData);
  void setData(void *_data) { data = _data; }

  Buffer getLocal();
  nxs_status copyData(void *_hostBuf, nxs_uint direction) const;
  nxs_status fillData(void *value, nxs_uint size_bytes) const;
  std::string print() const;

 private:
  typedef std::vector<char> StorageType;

  void *getVoidData() const;

  // set of runtimes
  nxs_ulong size_bytes;
  Shape shape;
  void *data;
};
}  // namespace detail
}  // namespace nexus

#endif  // _NEXUS_BUFFER_IMPL_H