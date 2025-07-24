#ifndef RT_CUDA_RUNTIME_H
#define RT_CUDA_RUNTIME_H

#include <rt_runtime.h>

#include <cuda_runtime.h>
#include <cuda_device.h>
#include <cuda_kernel.h>
#include <cuda_command.h>
#include <cuda_schedule.h>

#define CHECK_CU(call) \
  do { \
    CUresult err = call; \
    if (err != CUDA_SUCCESS) { \
      const char* errorStr; \
      cuGetErrorString(err, &errorStr); \
      std::cerr << "CUDA Error: " << errorStr << std::endl; \
      exit(1); \
    } \
  } while(0)

#define CHECK_CUDA(call) \
  do { \
    cudaError_t err = call; \
    if (err != cudaSuccess) { \
      std::cerr << "CUDA Runtime Error: " << cudaGetErrorString(err) << std::endl; \
      exit(1); \
    } \
  } while(0)

#include <nexus-api.h>

#define NXSAPI_LOG_MODULE "cuda_runtime"

using namespace nxs;

class CudaRuntime : public rt::Runtime {

public:

  nxs_int numDevices;
  nxs_int current_device = -1;
  rt::Pool<rt::Buffer> buffer_pool;
  rt::Pool<CudaCommand> command_pool;
  rt::Pool<CudaSchedule> schedule_pool;

  CudaRuntime() : rt::Runtime() {
    CUresult cuResult = cuInit(0);
    CHECK_CU(cuResult);

    setupCudaDevices();

    if (this->getNumObjects() == 0) {
      NXSAPI_LOG(NXSAPI_STATUS_ERR, "No Cuda devices found.");
      return;
    }

    numDevices = this->getNumObjects();

    NXSAPI_LOG(NXSAPI_STATUS_NOTE, "CUDA Runtime initialized with result: " << cuResult);
  }
  ~CudaRuntime() = default;
  template <typename T>
  T getPtr(nxs_int id) {
    return static_cast<T>(get(id));
  }

  void setupCudaDevices() {
    int deviceCount;
    cudaGetDeviceCount(&deviceCount);

    for (int i = 0; i < deviceCount; i++) {
      CudaDevice *device = new CudaDevice(i);
      addObject(device);
    }
  }

  nxs_int getDeviceCount() const {
    return numDevices;
  }

  CudaDevice *getDevice(nxs_int id) {
    if (id < 0 || id >= numDevices) return nullptr;
    if (id != current_device) {
      CHECK_CUDA(cudaSetDevice(id));
      current_device = id;
    }
    return get<CudaDevice>(id);
  }

  rt::Buffer *getBuffer(size_t size, void *cuda_buffer = nullptr) {
    return buffer_pool.get_new(size, cuda_buffer, false);
  }
  void release(rt::Buffer *buffer) { buffer_pool.release(buffer); }

  CudaCommand *getCommand(CUfunction kernel) {
    return command_pool.get_new(kernel);
  }

  CudaCommand *getCommand(cudaEvent_t event, nxs_command_type type,
                         nxs_int event_value = 0) {
    return command_pool.get_new(event, type, event_value);
  }

  void release(CudaCommand *cmd) { command_pool.release(cmd); }

  void release(CudaSchedule *sched) {
    sched->release(this);
    schedule_pool.release(sched);
  }
};

#endif  // RT_CUDA_RUNTIME_H
