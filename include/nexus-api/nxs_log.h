#ifndef NEXUS_API_LOG_H
#define NEXUS_API_LOG_H

#if defined(__cplusplus)
#include <nexus/log.h>

#define NXSAPI_LOG(SEVERITY, ...) \
  ::nexus::LogManager::getInstance().log(SEVERITY, "NXSAPI:" NXSAPI_LOG_MODULE, __VA_ARGS__)

#else
#define NXSAPI_LOG(SEVERITY, ...)
#endif

#endif  // NEXUS_API_LOG_H