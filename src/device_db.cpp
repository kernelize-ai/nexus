#include <dirent.h>
#include <nexus/device_db.h>
#include <nexus/log.h>
#include <nexus/utility.h>

#include <fstream>
#include <iostream>
#include <sstream>

using namespace nexus;

#define NEXUS_LOG_MODULE "device_info"

static bool initDeviceInfoDB(DeviceInfoMap &devs) {
  iterateEnvPaths("NEXUS_DEVICE_PATH", "./device_lib",
                  [&](const std::string &path, const std::string &name) {
                    NEXUS_LOG(NEXUS_STATUS_NOTE, "  File: " << name);
                    std::string::size_type const p(name.find_last_of('.'));
                    std::string basename = name.substr(0, p);
                    devs.emplace(basename, path);
                  });
  return true;
}

const DeviceInfoMap *nexus::getDeviceInfoDB() {
  static DeviceInfoMap s_device_info_map;
  static bool init = initDeviceInfoDB(s_device_info_map);
  return &s_device_info_map;
}

Info nexus::lookupDeviceInfo(const std::string &archName) {
  const DeviceInfoMap *devmap = getDeviceInfoDB();
  auto ii = devmap->find(archName);
  if (ii != devmap->end()) return ii->second;
  return Info();
}
