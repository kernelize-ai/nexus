#ifndef _NEXUS_INFO_IMPL_H
#define _NEXUS_INFO_IMPL_H

#include <nexus/info.h>

#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>
using json = nlohmann::json;

namespace nexus {

class Info::Node : public json {
 public:
  Node(json node = json::object()) : json(node) {}
  ~Node() = default;
  json &getJson() { return *this; }
  std::optional<Node> getNode(const std::string_view &name) const {
    try {
      return Node(this->at(name));
    } catch (...) {
      return std::nullopt;
    }
  }

  template <typename T>
  T get(const std::string_view &name) const {
    try {
      return this->at(name).get<T>();
    } catch (...) {
      return 0;
    }
  }
};

template <>
std::string_view Info::Node::get<std::string_view>(
    const std::string_view &name) const;

namespace detail {
class InfoImpl {
  std::string propertyFilePath;
  std::once_flag loaded;
  json props;

 public:
  InfoImpl(const std::string &filepath);
  InfoImpl(const Info::Node &node);
  std::optional<Property> getProperty(
      const std::vector<std::string_view> &propPath);
  Info::Node getNode(const std::vector<std::string_view> &path);

 private:
  nxs_int getIndex(const std::string_view &name) const;
  json getNode(const std::vector<std::string_view> &path) const;
  nxs_property_type getNodeType(json node) const;
  std::optional<Property> getValue(json node, nxs_int propTypeId) const;
  std::optional<Property> getKeys(json node) const;
  std::optional<Property> getProp(
      const std::vector<std::string_view> &path) const;

 private:
  void loadInfo();
};

}  // namespace detail
}  // namespace nexus

#endif  // _NEXUS_INFO_IMPL_H