#ifndef NEXUS_DEVICE_DB_H
#define NEXUS_DEVICE_DB_H

#include <nexus/info.h>

#include <optional>
#include <string>
#include <unordered_map>

namespace nexus {

typedef std::unordered_map<std::string, Info> DeviceInfoMap;

const DeviceInfoMap *getDeviceInfoDB();

Info lookupDeviceInfo(const std::string &archName);

}  // namespace nexus

#endif  // NEXUS_DEVICE_DB_H