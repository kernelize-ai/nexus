#ifndef RT_TT_DEVICE_H
#define RT_TT_DEVICE_H

#include "tenstorrent.h"

class TTDevice {
  int device_id;
  std::shared_ptr<ttmd::MeshDevice> device;
 public:
  TTDevice(int device_id = 0) : device_id(device_id) {}
  virtual ~TTDevice() { release(); }

  nxs_status release() {
    device = nullptr;
    return NXS_Success;
  }

  std::shared_ptr<ttmd::MeshDevice> get() { initDevice(); return device; }

  ttmd::MeshCommandQueue& getCQ() { initDevice(); return device->mesh_command_queue(); }

  ttmd::MeshCoordinateRange getRange() {
    initDevice();
    TT_NOBJ_CHECK(devRange, ttmd::MeshCoordinateRange, device->shape());
    return devRange;
  }

 private:
  bool initDevice() {
    if (device) return true;
    NXSAPI_LOG(nexus::NXS_LOG_NOTE, "Create TTDevice: ", device_id);
    TT_OBJ_CHECK(device, ttmd::MeshDevice::create_unit_mesh, device_id);
    return true;
  }
};

#endif  // RT_TT_DEVICE_H