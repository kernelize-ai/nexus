#ifndef NEXUS_INFO_H
#define NEXUS_INFO_H

#include <nexus-api.h>
#include <nexus/object.h>

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace nexus {

namespace detail {
class InfoImpl;
}
class Info : public Object<detail::InfoImpl> {
 public:
  class Node;

  Info(const std::string &filepath);
  Info(Node &node);
  Info() = default;

  // Query Device Properties
  //   from name
  std::optional<Property> getProperty(const std::string_view &prop) const;
  //   from path
  std::optional<Property> getProperty(
      const std::vector<std::string_view> &path) const;

  //   from name
  std::optional<Property> getProperty(nxs_int prop) const override {
    return getProperty(nxsGetPropName(prop));
  }

  //   from path
  std::optional<Property> getProperty(
      const std::vector<nxs_int> &propPath) const {
    std::vector<std::string_view> names;
    std::for_each(propPath.begin(), propPath.end(),
                  [&](nxs_int pn) { names.push_back(nxsGetPropName(pn)); });
    return getProperty(names);
  }

  std::optional<Node> getNode(const std::vector<std::string_view> &path) const;
};

typedef Objects<Info> Infos;

}  // namespace nexus

#endif  // NEXUS_INFO_H
