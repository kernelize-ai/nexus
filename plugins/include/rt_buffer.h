#ifndef RT_BUFFER_H
#define RT_BUFFER_H

#include <nexus-api.h>

#include <cstring>

namespace nxs {
namespace rt {

class Buffer {
  char *buf;
  size_t sz;
  nxs_uint settings;

 public:
  Buffer(size_t size = 0, void *data_ptr = nullptr, nxs_uint settings = 0)
      : buf((char *)data_ptr), sz(size), settings(settings) {
    if (settings & NXS_BufferSettings_Maintain) {
      buf = (char *)malloc(size);
      if (data_ptr) std::memcpy((void *)buf, data_ptr, size);
    }
  }
  ~Buffer() { release(); }
  void release() {
    if (buf && settings & NXS_BufferSettings_Maintain)
      free(buf);
    buf = nullptr;
    sz = 0;
  }
  char *data() const { return buf; }
  char *getData() const { return buf; }
  size_t size() const { return sz; }
  size_t getSize() const { return sz; }
  void setSize(size_t new_size) {
    auto *buf_c = buf;
    if (settings & NXS_BufferSettings_Maintain) {
      buf = (char *)realloc(buf, new_size);
    } else {
      settings |= NXS_BufferSettings_Maintain;
      buf = (char *)malloc(new_size);
      if (buf_c) std::memcpy((void *)buf, buf_c, std::min(sz, new_size));
    }
    sz = new_size;
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
