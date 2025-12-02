#ifndef RT_UTILITIES_H
#define RT_UTILITIES_H

#include <nexus-api.h>

#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>

namespace nxs {
namespace rt {

static nxs_status getPropertyStr(void *property_value,
                                 size_t *property_value_size, const char *name,
                                 size_t len) {
  if (property_value != NULL) {
    if (property_value_size == NULL)
      return NXS_InvalidArgSize;
    else if (*property_value_size < len)
      return NXS_InvalidArgValue;
    std::strncpy((char *)property_value, name, len + 1);
  }
  if (property_value_size != NULL) {
    *property_value_size = len + 1;
  }
  return NXS_Success;
}

static nxs_status getPropertyStr(void *property_value,
                                 size_t *property_value_size,
                                 const std::string &value) {
  return getPropertyStr(property_value, property_value_size, value.c_str(),
                        value.size());
}

static nxs_status getPropertyInt(void *property_value,
                                 size_t *property_value_size, nxs_long value) {
  if (property_value != NULL) {
    if (property_value_size == NULL)
      return NXS_InvalidArgSize;
    else if (*property_value_size < sizeof(value))
      return NXS_InvalidArgValue;
    std::memcpy(property_value, &value, sizeof(value));
  }
  if (property_value_size != NULL) {
    *property_value_size = sizeof(value);
  }
  return NXS_Success;
}

static nxs_status getPropertyFlt(void *property_value,
                                 size_t *property_value_size,
                                 nxs_double value) {
  if (property_value != NULL) {
    if (property_value_size == NULL)
      return NXS_InvalidArgSize;
    else if (*property_value_size < sizeof(value))
      return NXS_InvalidArgValue;
    std::memcpy(property_value, &value, sizeof(value));
  }
  if (property_value_size != NULL) {
    *property_value_size = sizeof(value);
  }
  return NXS_Success;
}

template <typename T>
static nxs_status getPropertyVec(void *property_value,
                                 size_t *property_value_size, const T *values,
                                 size_t num_values) {
  auto size = num_values * sizeof(T);
  if (property_value != NULL) {
    if (property_value_size == NULL)
      return NXS_InvalidArgSize;
    else if (*property_value_size < size)
      return NXS_InvalidArgValue;
    std::memcpy(property_value, values, size);
  }
  if (property_value_size != NULL) {
    *property_value_size = size;
  }
  return NXS_Success;
}

///////////////////////////////////////////////////////////////////////////////
//  Dump debug
template <typename T>
static std::string print_value(const T &value) {
  std::stringstream ss;
  ss << " - 0x" << std::hex << (nxs_ulong)value;
  return ss.str();
}

static std::string print_value() { return ""; }

template <typename T, typename... Args>
static std::string print_value(T value, Args... args) {
  return print_value(value) + print_value(args...);
}

}  // namespace rt
}  // namespace nxs

#endif  // RT_UTILITIES_H
