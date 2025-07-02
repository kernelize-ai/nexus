#ifndef RT_BUFFER_H
#define RT_BUFFER_H

#include <nexus-api.h>

namespace nxs {
namespace rt {

class Buffer {
  char *buf;
  size_t sz;
  bool is_owned;

 public:
  Buffer(size_t size, void *host_ptr = nullptr, bool is_owned = false)
      : buf((char *)host_ptr), sz(size), is_owned(is_owned) {
    if (host_ptr && is_owned) {
      buf = (char *)malloc(size);
      memcpy(buf, host_ptr, size);
    }
  }
  ~Buffer() {
    if (is_owned && buf) free(buf);
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