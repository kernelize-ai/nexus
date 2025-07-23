#ifndef RT_CUDA_DEVICE_H
#define RT_CUDA_DEVICE_H

#include <string>
#include <vector>
#include <regex>

#include <rt_runtime.h>
#include <rt_device.h>
#include <cuda_library.h>
#include <cuda_schedule.h>

using namespace nxs;

class CudaDevice : public rt::Device {

public:

  CUcontext context;
  CUdevice cudaDeviceRef;
  cudaDeviceProp props;

  Libraries libraries;

  CudaDevice(int deviceID) : nxs::rt::Device(deviceID, nullptr) {
    cudaGetDeviceProperties(&props, deviceID);
    cuDeviceGet(&cudaDeviceRef, 0);
    cuCtxCreate(&context, 0, cudaDeviceRef);
    libraries.reserve(1024);
  }
  ~CudaDevice() = default;

  CudaLibrary *createLibrary(void *library_data, nxs_uint data_size) {
    libraries.emplace_back(library_data, data_size);
    return &libraries.back();
  }

  CudaLibrary *createLibraryFromFile(const std::string &library_path) {
    libraries.emplace_back(library_path);
    return &libraries.back();
  }

  nxs_status copyBuffer(void *host_ptr, CudaBuffer *buffer_ptr) {
    CHECK_CUDA(cudaMemcpy(host_ptr, (float *)buffer_ptr->cudaPtr, buffer_ptr->size(), cudaMemcpyDeviceToHost));
    return NXS_Success;
  }

  nxs_status runSchedule(CudaSchedule *schedule) {
    auto commands = schedule->getCommands();
    for (auto command : commands) {
      CUfunction func = command->cudaKernel->kernel;
      if (func == nullptr)
        return NXS_InvalidKernel;

      std::vector<void*> kernel_args;
      for (auto& buffer : command->buffers)
        kernel_args.push_back(&buffer->cudaPtr);

      int n_val = 1024;
      kernel_args.push_back(&n_val);
      CUresult cu_result = cuLaunchKernel(func,
                                        command->gridSize, 1, 1,
                                        command->blockSize, 1, 1,
                                        0, nullptr,
                                        kernel_args.data(),
                                        nullptr);

      if (cu_result != CUDA_SUCCESS) {
        const char* error_string;
        cuGetErrorString(cu_result, &error_string);
        std::cout << "Kernel launch failed: " << error_string << std::endl;
        return NXS_InvalidKernel;
      }
      cudaDeviceSynchronize();
    }
    return NXS_Success;
  }
};

#endif // RT_CUDA_DEVICE_H
