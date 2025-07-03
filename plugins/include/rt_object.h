#ifndef RT_OBJECT_H
#define RT_OBJECT_H

#include <nexus-api.h>

#include <cassert>
#include <functional>

namespace nxs {
namespace rt {

typedef std::function<void(void *)> release_fn_t;

template <typename T>
void delete_fn(void *obj) {
  delete static_cast<T *>(obj);
}

class Object {
  Object *parent;
  void *obj;
  bool is_owned;
  typedef std::vector<nxs_int> children_t;
  children_t children;

 public:
  Object(Object *parent = nullptr, void *obj = nullptr, bool is_owned = true) {
    this->parent = parent;
    this->obj = obj;
    this->is_owned = obj ? is_owned : false;
  }
  virtual ~Object() {
    // assert(is_owned == false);
  }

  template <typename T = void>
  T *get() const {
    return static_cast<T *>(obj);
  }

  void release(release_fn_t fn) {
    children.clear();
    if (is_owned && obj) {
      assert(fn);  // @@@
      fn(obj);
    }
    obj = nullptr;
    is_owned = false;
  }
  Object *getParent() { return parent; }
  children_t &getChildren() { return children; }
  void addChild(nxs_int child, nxs_int index = -1) {
    if (index < 0)
      children.push_back(child);
    else {
      if (index >= children.size()) children.resize(index + 1);
      children[index] = child;
    }
  }
};

}  // namespace rt
}  // namespace nxs

#endif  // RT_OBJECT_H