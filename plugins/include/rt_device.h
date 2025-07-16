#ifndef RT_DEVICE_H
#define RT_DEVICE_H

#include <nexus-api.h>

#include <cstring>

namespace nxs {
namespace rt {

class Device : public Object {

 public:

  std::string name;
  std::string uuid;
  int  busID = -1;
  int deviceID = -1;

  Device(char* name, char* uuid, int busID, int deviceID, Object *runtime = nullptr) 
    : Object(runtime), name(name), uuid(uuid), busID(busID), deviceID(deviceID) {}
  virtual ~Device() = default;

  static void delete_fn(void *obj) {
    delete static_cast<Device *>(obj);
  }
};

}  // namespace rt
}  // namespace nxs

#endif  // RT_DEVICE_H