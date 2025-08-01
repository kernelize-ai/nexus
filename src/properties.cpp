
#include <nexus/log.h>
#include <nexus/properties.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <fstream>
#include <mutex>

using namespace nexus;
using namespace nexus::detail;

#define NEXUS_LOG_MODULE "properties"

namespace nexus {
namespace detail {
class PropertiesImpl {
  std::string propertyFilePath;
  std::once_flag loaded;
  json props;

 public:
  PropertiesImpl(const std::string &filepath) : propertyFilePath(filepath) {}

  std::optional<Property> getProperty(
      const std::vector<std::string> &propPath) {
    std::call_once(loaded, [&]() { loadProperties(); });
    return getProp(propPath);
  }

 private:
  nxs_int getIndex(const std::string &name) const {
    try {
      size_t num = 0;
      return std::stoi(name, &num);
    } catch (...) {
    }
    return nxsGetPropEnum(name.c_str());
  }

  json getNode(const std::vector<std::string> &path) const {
    json node = props;
    auto end = path.end() - 1;
    for (auto ii = path.begin(); ii != end; ++ii) {
      auto &key = *ii;
      if (node.is_array())
        node = node[getIndex(key)];
      else
        node = node.at(key);
    }
    return node;
  }

  nxs_property_type getNodeType(json node) const {
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

  std::optional<Property> getValue(json node, nxs_int propTypeId) const {
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

  std::optional<Property> getKeys(json node) const {
    if (node.is_object()) {
      std::vector<std::string> keys;
      for (auto &elem : node.items()) keys.push_back(elem.key());
      return Property(keys);
    }
    return std::nullopt;
  }

  std::optional<Property> getProp(const std::vector<std::string> &path) const {
    if (!path.empty()) {
      try {
        auto tail = path.back();
        auto typeId = nxsGetPropEnum(tail.c_str());
        // NEXUS_LOG(NEXUS_STATUS_NOTE,
        //           "  Properties.getProp - " << tail << " - " << typeId);
        auto node = getNode(path);
        if (node.is_object()) {
          if (tail == "Keys") return getKeys(node);
        } else if (node.is_array()) {
          if (tail == "Size") return Property((nxs_long)node.size());
          // get elem
          return getValue(node[getIndex(tail)], typeId);
        }
        return getValue(node.at(tail), typeId);
      } catch (...) {
        NEXUS_LOG(NEXUS_STATUS_ERR, "  Properties.getProp - " << path[0]);
      }
    }
    return std::nullopt;
  }

 private:
  void loadProperties() {
    // Load json from file
    try {
      std::ifstream f(propertyFilePath);
      props = json::parse(f);
      NEXUS_LOG(NEXUS_STATUS_NOTE, "Loaded json from "
                                       << propertyFilePath
                                       << " - size: " << props.size());
    } catch (...) {
      NEXUS_LOG(NEXUS_STATUS_ERR, "Failed to load " << propertyFilePath);
    }
  }
};

}  // namespace detail
}  // namespace nexus

///////////////////////////////////////////////////////////////////////////////
/// @brief
///////////////////////////////////////////////////////////////////////////////
Properties::Properties(const std::string &filepath) : Object(filepath) {}

// Get top level node
std::optional<Property> Properties::getProperty(const std::string &name) const {
  std::vector<std::string> path{name};
  NEXUS_OBJ_MCALL(std::nullopt, getProperty, path);
}

// Get sub-node
std::optional<Property> Properties::getProperty(
    const std::vector<std::string> &path) const {
  NEXUS_OBJ_MCALL(std::nullopt, getProperty, path);
}
