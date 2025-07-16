#ifndef RT_RUNTIME_H
#define RT_RUNTIME_H

#include <optional>

#include <nexus-api.h>
#include <rt_object.h>

namespace nxs {
namespace rt {

class Runtime : public Object {

public:

  std::vector<rt::Object> objects;

  Runtime() { objects.reserve(1024); }
  ~Runtime() {}

  nxs_int addObject(Object *parent, void *obj = nullptr, bool is_owned = false) {
    objects.emplace_back(parent, obj, is_owned);
    return objects.size() - 1;
  }

  std::optional<rt::Object *> getObject(nxs_int id) {
    if (id < 0 || id >= objects.size()) return std::nullopt;
    return &objects[id];
  }

  template <typename T = void>
  std::optional<T *> get(nxs_int id) {
    if (auto obj = getObject(id)) return (*obj)->get<T>();
    return std::nullopt;
  }

  bool dropObject(nxs_int id, release_fn_t fn = nullptr) {
    if (id < 0 || id >= objects.size()) return false;
    objects[id].release(fn);
    return true;
  }

  nxs_int getNumObjects() {
    return objects.size();
  }
};

}  // namespace rt
}  // namespace nxs

#endif  // RT_RUNTIME_H