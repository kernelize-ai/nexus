#ifndef RT_UTILITIES_H
#define RT_UTILITIES_H

#include <nexus-api.h>

#include <string>

static nxs_status rt_getPropertyStr(void *property_value,
                                    size_t *property_value_size,
                                    const char *name, size_t len) {
  if (property_value != NULL) {
    if (property_value_size == NULL)
      return NXS_InvalidArgSize;
    else if (*property_value_size < len)
      return NXS_InvalidArgValue;
    strncpy((char *)property_value, name, len + 1);
  } else if (property_value_size != NULL) {
    *property_value_size = len + 1;
  }
  return NXS_Success;
}

static nxs_status rt_getPropertyStr(void *property_value,
                                    size_t *property_value_size,
                                    const std::string &value) {
  return rt_getPropertyStr(property_value, property_value_size, value.c_str(),
                           value.size());
}

static nxs_status rt_getPropertyInt(void *property_value,
                                    size_t *property_value_size,
                                    nxs_long value) {
  if (property_value != NULL) {
    if (property_value_size == NULL)
      return NXS_InvalidArgSize;
    else if (*property_value_size < sizeof(value))
      return NXS_InvalidArgValue;
    memcpy(property_value, &value, sizeof(value));
  } else if (property_value_size != NULL) {
    *property_value_size = sizeof(value);
  }
  return NXS_Success;
}

#endif  // RT_UTILITIES_H