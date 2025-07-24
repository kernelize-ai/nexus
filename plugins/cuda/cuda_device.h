#ifndef RT_CUDA_DEVICE_H
#define RT_CUDA_DEVICE_H

#include <string>
#include <vector>

#include <cuda_buffer.h>
#include <cuda_library.h>
#include <cuda_kernel.h>

using namespace nxs;

class CudaDevice {

public:

  CUcontext context;
  CUdevice cudaDeviceRef;
  cudaDeviceProp props;

  Libraries libraries;

  CudaDevice(int deviceID) {
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
};

#endif // RT_CUDA_DEVICE_H
