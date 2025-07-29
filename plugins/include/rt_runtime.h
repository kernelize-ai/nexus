#ifndef RT_RUNTIME_H
#define RT_RUNTIME_H

#include <nexus-api.h>
#include <rt_object.h>
#include <rt_pool.h>

#include <optional>

namespace nxs {
namespace rt {

class Runtime {
  Pool<rt::Object> objects;

public:
 Runtime() {}
 ~Runtime() {}

 nxs_int addObject(void *obj = nullptr, bool is_owned = false) {
   return objects.acquire(obj, is_owned);
  }
  nxs_int addObject(nxs_long value) { return objects.acquire(value); }

  std::optional<rt::Object *> getObject(nxs_int id) {
    if (id < 0 || id >= objects.capacity()) return std::nullopt;
    return objects.get(id);
  }

  template <typename T = void>
  T *get(nxs_int id) {
    if (auto obj = getObject(id)) return (*obj)->get<T>();
    return nullptr;
  }
  nxs_long getValue(nxs_int id) {
    if (auto obj = getObject(id)) return (*obj)->getValue();
    return 0;
  }

  bool dropObject(nxs_int id, release_fn_t fn = nullptr) {
    if (id < 0 || id >= objects.capacity()) return false;
    auto obj = objects.get(id);
    if (fn) fn(obj->get());
    objects.release(id);
    return true;
  }

  nxs_int getNumObjects() { return objects.get_in_use_count(); }
};

}  // namespace rt
}  // namespace nxs

#endif  // RT_RUNTIME_H
