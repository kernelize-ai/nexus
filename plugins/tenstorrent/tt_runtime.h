#ifndef RT_TT_RUNTIME_H
#define RT_TT_RUNTIME_H

#include "tenstorrent.h"

#include <tt_command.h>
#include <tt_runtime.h>
#include <tt_schedule.h>
#include <tt_library.h>
#include <tt_buffer.h>
#include <rt_runtime.h>

using namespace nxs;

class TTRuntime : public rt::Runtime {
  std::vector<TTDevice> devices;
  rt::Pool<TTBuffer, 256> buffer_pool;
  rt::Pool<TTCommand> command_pool;
  rt::Pool<TTSchedule, 256> schedule_pool;
  rt::Pool<TTLibrary, 64> library_pool;

 public:
  TTRuntime() : rt::Runtime() {
    TT_NOBJ_CHECK(numDevs, ttm::GetNumAvailableDevices);
    NXSAPI_LOG(nexus::NXS_LOG_NOTE, "Create TTDevice count: ", numDevs);
    for (size_t i = 0; i < numDevs; ++i) {
      NXSAPI_LOG(nexus::NXS_LOG_NOTE, "Create TTDevice: ", i);
        //   static std::shared_ptr<MeshDevice> create_unit_mesh(
        //     int device_id,
        //     size_t l1_small_size = DEFAULT_L1_SMALL_SIZE,
        //     size_t trace_region_size = DEFAULT_TRACE_REGION_SIZE,
        //     size_t num_command_queues = 1,
        //     const DispatchCoreConfig& dispatch_core_config = DispatchCoreConfig{},
        //     tt::stl::Span<const std::uint32_t> l1_bank_remap = {}, 
        //     size_t worker_l1_size = DEFAULT_WORKER_L1_SIZE);
      TT_NOBJ_CHECK(device, ttmd::MeshDevice::create_unit_mesh, i);
      TT_NOBJ_CHECK(&cq, device->mesh_command_queue);
      devices.push_back(device);
      addObject(i);
    }
  }
  ~TTRuntime() {
    NXSAPI_LOG(nexus::NXS_LOG_NOTE, "Close TT Devices");
    devices.clear();
  }

  nxs_int getNumDevices() const { return devices.size(); }

  TTDevice getDevice(nxs_int device_id) {
    return devices[device_id];
  }

  template <typename T>
  T getPtr(nxs_int id) {
    return static_cast<T>(get(id));
  }

  TTBuffer *getBuffer(TTDevice device, size_t size, void *data_ptr = nullptr,
                        bool copy_data = false) {
    return buffer_pool.get_new(device, size, data_ptr, copy_data);
  }
  nxs_status releaseBuffer(nxs_int buffer_id) {
    auto buf = get<TTBuffer>(buffer_id);
    if (!buf) return NXS_InvalidBuffer;
    buffer_pool.release(buf);
    if (!dropObject(buffer_id)) return NXS_InvalidBuffer;
    return NXS_Success;
  }

  nxs_int getSchedule(TTDevice device, nxs_uint settings = 0) {
    auto schedule = schedule_pool.get_new(device, settings);
    if (!schedule) return NXS_InvalidSchedule;
    return addObject(schedule);
  }

  TTCommand *getCommand(TTKernel *kernel, nxs_uint settings = 0) {
    return command_pool.get_new(this, kernel, settings);
  }
#if 0
  TTCommand *getCommand(nxs_int event, nxs_command_type type,
                         nxs_int event_value = 0, nxs_uint settings = 0) {
    return command_pool.get_new(this, event, type, event_value, settings);
  }
#endif

  nxs_status releaseCommand(nxs_int command_id) {
    auto cmd = get<TTCommand>(command_id);
    if (!cmd) return NXS_InvalidCommand;
    // cmd->release();
    command_pool.release(cmd);
    if (!dropObject(command_id)) return NXS_InvalidCommand;
    return NXS_Success;
  }

  TTLibrary *getLibrary(const std::string &filename, nxs_uint settings = 0) {
    return library_pool.get_new(this, filename, settings);
  }
  nxs_status releaseLibrary(nxs_int library_id) {
    auto lib = get<TTLibrary>(library_id);
    if (!lib) return NXS_InvalidLibrary;
    // cmd->release();
    library_pool.release(lib);
    if (!dropObject(library_id)) return NXS_InvalidCommand;
    return NXS_Success;
  }

  nxs_status releaseSchedule(nxs_int schedule_id) {
    auto sched = get<TTSchedule>(schedule_id);
    if (!sched) return NXS_InvalidSchedule;
    for (auto cmd : sched->getCommands()) {
      // releaseCommand(cmd->getId());
    }
    sched->release();
    schedule_pool.release(sched);
    if (!dropObject(schedule_id)) return NXS_InvalidSchedule;
    return NXS_Success;
  }
};

#endif  // RT_TT_RUNTIME_H
