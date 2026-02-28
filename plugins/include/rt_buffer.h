#ifndef RT_BUFFER_H
#define RT_BUFFER_H

#include <nexus-api.h>

#include <cstring>

namespace nxs {
namespace rt {

class Buffer {
  char *buf;
  nxs_shape shape;
  size_t size_bytes;
  nxs_uint settings;

 public:
  Buffer(const nxs_shape &shape, void *data_ptr = nullptr, nxs_uint settings = 0)
      : buf((char *)data_ptr), shape(shape), size_bytes(0), settings(settings) {
    setSizeBytes();
    if (settings & NXS_BufferSettings_Maintain) {
      buf = (char *)malloc(getSizeBytes());
      if (data_ptr) std::memcpy((void *)buf, data_ptr, getSizeBytes());
    }
  }
  Buffer(size_t size=0, void *data_ptr = nullptr, nxs_uint settings = 0)
      : Buffer(nxs_shape{{size}, (nxs_uint)(size == 0 ? 0 : 1)}, data_ptr, settings) {}
  ~Buffer() { release(); }
  void release() {
    if (buf && settings & NXS_BufferSettings_Maintain)
      free(buf);
    buf = nullptr;
    shape = nxs_shape{{0}, 0};
    size_bytes = 0;
  }
  char *data() const { return buf; }
  char *getData() const { return buf; }
  size_t getSizeBytes() const { return size_bytes; }
  void setSizeBytes() {
    size_bytes = getNumElements();
    if (auto element_size_bits = getElementSizeBits()) {
      size_bytes *= element_size_bits;
      size_bytes /= 8;
    }
  }
  void setSizeBytes(size_t new_size_bytes) {
    auto *buf_c = buf;
    if (settings & NXS_BufferSettings_Maintain) {
      buf = (char *)realloc(buf, new_size_bytes);
    } else {
      settings |= NXS_BufferSettings_Maintain;
      buf = (char *)malloc(new_size_bytes);
      if (buf_c) std::memcpy((void *)buf, buf_c, std::min(size_bytes, new_size_bytes));
    }
    shape = nxs_shape{{new_size_bytes}, 1};
    size_bytes = new_size_bytes;
  }
  const nxs_shape& getShape() const { return shape; }
  nxs_ulong getNumElements() const { return nxsGetNumElements(shape); }
  nxs_uint getElementSizeBits() const {
    return nxsGetDataTypeSizeBits(settings);
  }
  nxs_uint getSettings() const { return settings; }
  void setSettings(nxs_uint new_settings) {
    settings = new_settings;
  }
  template <typename T = void>
  T *get() {
    return reinterpret_cast<T *>(buf);
  }
};

}  // namespace rt
}  // namespace nxs

#endif  // RT_BUFFER_H
