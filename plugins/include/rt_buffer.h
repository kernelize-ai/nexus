#ifndef RT_BUFFER_H
#define RT_BUFFER_H

#include <cstring>

#include <nexus-api.h>
#include <rt_object.h>

namespace nxs {
namespace rt {

class Buffer : public Object {

  char *buf;
  size_t sz;
  bool is_owned;

 public:
  Buffer(Object *parent, size_t size, void *host_ptr = nullptr, bool is_owned = false)
      : Object(parent), buf((char *)host_ptr), sz(size), is_owned(is_owned) {
    if (is_owned) {
      buf = (char *)malloc(size);
      if (host_ptr)
        std::memcpy((void *)buf, host_ptr, size);
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
  static void delete_fn(void *obj) {
    delete static_cast<Buffer *>(obj);
  }
};

typedef std::vector<Buffer> Buffers;

}  // namespace rt
}  // namespace nxs

#endif  // RT_BUFFER_H