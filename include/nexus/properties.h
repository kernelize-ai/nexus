#ifndef NEXUS_PROPERTIES_H
#define NEXUS_PROPERTIES_H

#include <nexus-api.h>
#include <nexus/object.h>

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace nexus {

namespace detail {
class PropertiesImpl;
}
class Properties : public Object<detail::PropertiesImpl> {
 public:
  Properties(const std::string &filepath);
  using Object::Object;

  // Query Device Properties
  //   from name
  std::optional<Property> getProperty(const std::string &prop) const;
  //   from path
  std::optional<Property> getProperty(
      const std::vector<std::string> &path) const;

  //   from name
  std::optional<Property> getProperty(nxs_int prop) const override {
    return getProperty(nxsGetPropName(prop));
  }

  //   from path
  std::optional<Property> getProperty(
      const std::vector<nxs_int> &propPath) const {
    std::vector<std::string> names;
    std::for_each(propPath.begin(), propPath.end(),
                  [&](nxs_int pn) { names.push_back(nxsGetPropName(pn)); });
    return getProperty(names);
  }
};

}  // namespace nexus

#endif  // NEXUS_PROPERTIES_H
