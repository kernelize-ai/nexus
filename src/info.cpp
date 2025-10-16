
#include <nexus/log.h>
#include <nexus/info.h>

#include <fstream>
#include <mutex>

#include "_info_impl.h"

using namespace nexus;
using namespace nexus::detail;

#define NEXUS_LOG_MODULE "info"

namespace nexus {

template <>
std::string_view Info::Node::get<std::string_view>(
    const std::string_view &name) const {
  try {
    return this->at(name).get<std::string_view>();
  } catch (...) {
    return std::string_view();
  }
}

namespace detail {

InfoImpl::InfoImpl(const std::string &filepath)
    : propertyFilePath(filepath) {}
InfoImpl::InfoImpl(Info::Node &node) {
  props = node.getJson();
  NEXUS_LOG(NXS_LOG_NOTE, "  JSON size: ", props.size());
}

std::optional<Property> InfoImpl::getProperty(
    const std::vector<std::string_view> &propPath) {
  std::call_once(loaded, [&]() { loadInfo(); });
  return getProp(propPath);
}

Info::Node InfoImpl::getNode(
    const std::vector<std::string_view> &path) {
  std::call_once(loaded, [&]() { loadInfo(); });
  json node = props;
  for (auto &key : path) {
    if (node.is_array())
      node = node[getIndex(key.data())];
    else
      node = node.at(key.data());
  }
  return Info::Node(node);
}

nxs_int InfoImpl::getIndex(const std::string_view &name) const {
  if (name.empty()) return 0;
  char *end = nullptr;
  auto num = strtol(name.data(), &end, 10);
  if (end == name.data() + name.size()) {
    return num;
  }
  return nxsGetPropEnum(name.data());
}

json InfoImpl::getNode(const std::vector<std::string_view> &path) const {
  json node = props;
  auto end = path.end() - 1;
  for (auto ii = path.begin(); ii != end; ++ii) {
    auto &key = *ii;
    if (node.is_array())
      node = node[getIndex(key.data())];
    else
      node = node.at(key.data());
  }
  return node;
}

nxs_property_type InfoImpl::getNodeType(json node) const {
  if (node.is_array())
    return (nxs_property_type)(NPT_INT_VEC + getNodeType(node[0]));
  else if (node.is_string())
    return NPT_STR;
  else if (node.is_boolean())
    return NPT_INT;
  else if (node.is_number_float())
    return NPT_FLT;
  else if (node.is_number())
    return NPT_INT;
  return NPT_UNK;
}

std::optional<Property> InfoImpl::getValue(json node,
                                           nxs_int propTypeId) const {
  nxs_property_type propType = NPT_UNK;
  if (nxs_success(propTypeId)) {
    propType = nxs_property_type_map[propTypeId];
  } else {
    // get from json type
    propType = getNodeType(node);
  }
  // NEXUS_LOG(NEXUS_STATUS_NOTE, "  Properties.getValue - " << propType);
  switch (propType) {
    case NPT_INT:
      return Property(node.get<nxs_long>());
    case NPT_FLT:
      return Property(node.get<nxs_double>());
    case NPT_STR:
      return Property(node.get<std::string>());
    default:
      break;
  }
  return std::nullopt;
}

std::optional<Property> InfoImpl::getKeys(json node) const {
  if (node.is_object()) {
    std::vector<std::string> keys;
    for (auto &elem : node.items()) keys.push_back(elem.key());
    return Property(keys);
  }
  return std::nullopt;
}

std::optional<Property> InfoImpl::getProp(
    const std::vector<std::string_view> &path) const {
  if (!path.empty()) {
    try {
      auto tail = path.back();
      auto typeId = nxsGetPropEnum(tail.data());
      // NEXUS_LOG(NEXUS_STATUS_NOTE,
      //           "  Properties.getProp - " << tail << " - " << typeId);
      auto node = getNode(path);
      if (node.is_object()) {
        if (tail == "Keys") return getKeys(node);
      } else if (node.is_array()) {
        if (tail == "Size") return Property((nxs_long)node.size());
        // get elem
        return getValue(node[getIndex(tail.data())], typeId);
      }
      return getValue(node.at(tail.data()), typeId);
    } catch (...) {
      NEXUS_LOG(NXS_LOG_ERROR, "  Properties.getProp - ", path[0]);
    }
  }
  return std::nullopt;
}

void InfoImpl::loadInfo() {
  if (propertyFilePath.empty()) return;
  // Load json from file
  try {
    std::ifstream f(propertyFilePath);
    props = json::parse(f);
    NEXUS_LOG(NXS_LOG_NOTE, "Loaded json from "
                                     , propertyFilePath
                                     , " - size: ", props.size());
  } catch (...) {
    NEXUS_LOG(NXS_LOG_ERROR, "Failed to load ", propertyFilePath);
  }
}

}  // namespace detail
}  // namespace nexus

///////////////////////////////////////////////////////////////////////////////
/// @brief
///////////////////////////////////////////////////////////////////////////////
Info::Info(const std::string &filepath) : Object(filepath) {}

Info::Info(Node &node) : Object(node) {}

// Get top level node
std::optional<Property> Info::getProperty(const std::string_view &name) const {
  std::vector<std::string_view> path{name};
  NEXUS_OBJ_MCALL(std::nullopt, getProperty, path);
}

// Get sub-node
std::optional<Property> Info::getProperty(
    const std::vector<std::string_view> &path) const {
  NEXUS_OBJ_MCALL(std::nullopt, getProperty, path);
}

std::optional<Info::Node> Info::getNode(
    const std::vector<std::string_view> &path) const {
  NEXUS_OBJ_MCALL(std::nullopt, getNode, path);
}
