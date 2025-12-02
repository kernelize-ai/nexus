#ifndef RT_OBJECT_H
#define RT_OBJECT_H

#include <nexus-api.h>

#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <variant>
#include <vector>

namespace nxs {
namespace rt {

typedef std::function<void(void *)> release_fn_t;

template <typename T>
void delete_fn(void *obj) {
  delete static_cast<T *>(obj);
}

class Object {
  std::variant<void *, nxs_long> obj;
  bool is_owned;

 public:
  Object(void *_obj, bool _is_owned) {
    obj = _obj;
    is_owned = _obj ? _is_owned : false;
  }
  Object(nxs_long value = 0) {
    obj = value;
    is_owned = false;
  }
  virtual ~Object() {
    // assert(is_owned == false);
  }

  template <typename T = void>
  T *get() const {
    return static_cast<T *>(std::get<void *>(obj));
  }
  nxs_long getValue() const { return std::get<nxs_long>(obj); }

  void release(release_fn_t fn) {
    if (is_owned && std::holds_alternative<void *>(obj)) {
      assert(fn);  // @@@
      fn(std::get<void *>(obj));
    }
    obj = nullptr;
    is_owned = false;
  }
};

}  // namespace rt
}  // namespace nxs

#endif  // RT_OBJECT_H
