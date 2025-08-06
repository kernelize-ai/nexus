#ifndef RT_CUDA_RUNTIME_H
#define RT_CUDA_RUNTIME_H

#include <cuda_command.h>
#include <cuda_device.h>
#include <cuda_runtime.h>
#include <cuda_schedule.h>
#include <cuda_utils.h>
#include <rt_runtime.h>

#include <nvml.h>

class CudaRuntime : public rt::Runtime {

public:

  nxs_int numDevices;
  nxs_int current_device = -1;
  rt::Pool<rt::Buffer, 256> buffer_pool;
  rt::Pool<CudaCommand> command_pool;
  rt::Pool<CudaSchedule, 256> schedule_pool;

  CudaRuntime() : rt::Runtime() { setupCudaDevices(); }
  ~CudaRuntime() = default;

  template <typename T>
  T getPtr(nxs_int id) {
    return static_cast<T>(get(id));
  }

  nxs_status setupCudaDevices() {
    CU_CHECK(NXS_InvalidDevice, cuInit, 0);
    nvmlReturn_t result = nvmlInit();
    if (result != NVML_SUCCESS)
      return NXS_InvalidDevice;

    CUDA_CHECK(NXS_InvalidDevice, cudaGetDeviceCount, &numDevices);

    if (numDevices == 0) {
      NXSAPI_LOG(NXSAPI_STATUS_ERR, "No CUDA devices found.");
      return NXS_InvalidDevice;
    }

    for (int i = 0; i < numDevices; i++) {
      CudaDevice *device = new CudaDevice(i);
      addObject(device);
    }
    return NXS_Success;
  }

  nxs_int getDeviceCount() const {
    return numDevices;
  }

  CudaDevice *getDevice(nxs_int id) {
    if (id < 0 || id >= numDevices) return nullptr;
    if (id != current_device) {
      if (cudaSetDevice(id) != cudaSuccess) {
        NXSAPI_LOG(NXSAPI_STATUS_ERR, "Failed to set CUDA device " << id);
        return nullptr;
      }
      current_device = id;
    }
    return get<CudaDevice>(id);
  }

  rt::Buffer *getBuffer(size_t size, void *data_ptr = nullptr,
                        bool copy_data = false) {
    return buffer_pool.get_new(size, data_ptr, copy_data);
  }
  void release(rt::Buffer *buffer) { buffer_pool.release(buffer); }

  CudaSchedule *getSchedule(nxs_int device_id, nxs_uint settings = 0) {
    return schedule_pool.get_new(device_id, settings);
  }

  CudaCommand *getCommand(CUfunction kernel, nxs_uint settings = 0) {
    return command_pool.get_new(kernel, settings);
  }

  CudaCommand *getCommand(cudaEvent_t event, nxs_command_type type,
                          nxs_int event_value = 0, nxs_uint settings = 0) {
    return command_pool.get_new(event, type, event_value, settings);
  }

  void release(CudaCommand *cmd) { command_pool.release(cmd); }

  void release(CudaSchedule *sched) {
    sched->release();
    schedule_pool.release(sched);
  }
};

#endif  // RT_CUDA_RUNTIME_H
