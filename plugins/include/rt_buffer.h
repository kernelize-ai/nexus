#ifndef RT_BUFFER_H
#define RT_BUFFER_H

#include <nexus-api.h>

#include <cstring>

namespace nxs {
namespace rt {

class Buffer {
  char *buf;
  size_t sz;
  bool copy_data;

 public:
  Buffer(size_t size, void *data_ptr = nullptr, bool copy_data = false)
      : buf((char *)data_ptr), sz(size), copy_data(copy_data) {
    if (copy_data) {
      buf = (char *)malloc(size);
      if (data_ptr) std::memcpy((void *)buf, data_ptr, size);
    }
  }
  ~Buffer() { release(); }
  void release() {
    if (copy_data && buf) free(buf);
    buf = nullptr;
    sz = 0;
    copy_data = false;
  }
  char *data() { return buf; }
  size_t size() { return sz; }
  template <typename T = void>
  T *get() {
    return reinterpret_cast<T *>(buf);
  }
};

}  // namespace rt
}  // namespace nxs

#endif  // RT_BUFFER_H
