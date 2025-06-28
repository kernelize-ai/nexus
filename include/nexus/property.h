#ifndef NEXUS_PROPERTY_H
#define NEXUS_PROPERTY_H

#include <nexus-api.h>

#include <string>
#include <variant>
#include <vector>

/// @brief  NOT USED, see json in Device or nexus-api
namespace nexus {

using PropIntVec = std::vector<nxs_long>;
using PropFltVec = std::vector<nxs_double>;
using PropStrVec = std::vector<std::string>;

using PropVariant = std::variant<nxs_long, nxs_double, std::string, PropStrVec, PropIntVec, PropFltVec>;

class Property : public PropVariant {
 public:
  using PropVariant::PropVariant;

  template <nxs_property Tnp>
  typename nxsPropertyType<Tnp>::type getValue() const {
    return std::get<typename nxsPropertyType<Tnp>::type>(*this);
  }

  template <typename T>
  T getValue() const {
    return std::get<T>(*this);
  }

  template <typename T>
  std::vector<T> getValueVec() const {
    return std::get<std::vector<T>>(*this);
  }
};

}  // namespace nexus

#endif  // NEXUS_PROPERTY_H
