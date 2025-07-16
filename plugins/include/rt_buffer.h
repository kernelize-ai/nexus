#ifndef RT_BUFFER_H
#define RT_BUFFER_H

#include <nexus-api.h>

#include <cstring>

namespace nxs {
namespace rt {

class Buffer {
  char *buf;
  size_t sz;
  bool is_owned;

 public:
  Buffer(size_t size, void *host_ptr = nullptr, bool is_owned = false)
      : buf((char *)host_ptr), sz(size), is_owned(is_owned) {
    if (is_owned) {
      buf = (char *)malloc(size);
      if (host_ptr)
        std::memcpy((void *)buf, host_ptr, size);
    }
  }
  ~Buffer() { release(); }
  void release() {
    if (is_owned && buf) free(buf);
    buf = nullptr;
    sz = 0;
    is_owned = false;
  }
  char *data() { return buf; }
  size_t size() { return sz; }
  template <typename T>
  T *get() {
    return reinterpret_cast<T *>(buf);
  }
};

}  // namespace rt
}  // namespace nxs

#endif  // RT_BUFFER_H
