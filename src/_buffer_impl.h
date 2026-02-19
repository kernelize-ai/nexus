#ifndef _NEXUS_BUFFER_IMPL_H
#define _NEXUS_BUFFER_IMPL_H

#include <nexus/device.h>

namespace nexus {
namespace detail {
class BufferImpl : public Impl {
 public:
  BufferImpl(Impl base, size_t _sz, const char *_hostData);
  BufferImpl(Impl base, nxs_int _devId, size_t _sz, const char *_hostData);

  ~BufferImpl();

  void release();

  nxs_int getDeviceId() const { return deviceId; }

  std::optional<Property> getProperty(nxs_int prop) const;

  size_t getSize() const { return size; }
  const char *getData() const;
  nxs_data_type getDataType() const;
  size_t getNumElements() const;
  size_t getElementSize() const;

  void setData(size_t sz, const char *hostData);
  void setData(void *_data) { data = _data; }

  Buffer getLocal();
  nxs_status copyData(void *_hostBuf, nxs_uint direction) const;
  nxs_status fillData(void *value, size_t size) const;
  std::string print() const;

 private:
  typedef std::vector<char> DataBuf;

  void *getVoidData() const;

  // set of runtimes
  nxs_int deviceId;
  size_t size;
  void *data;
};
}  // namespace detail
}  // namespace nexus

#endif  // _NEXUS_BUFFER_IMPL_H