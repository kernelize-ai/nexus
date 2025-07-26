#ifndef RT_CUDA_DEVICE_H
#define RT_CUDA_DEVICE_H

#include <cuda_utils.h>

class CudaDevice {

public:

  CUcontext context;
  CUdevice cudaDeviceRef;
  cudaDeviceProp props;

  CudaDevice(int deviceID) {
    cudaGetDeviceProperties(&props, deviceID);
    cuDeviceGet(&cudaDeviceRef, 0);
    cuCtxCreate(&context, 0, cudaDeviceRef);
  }
  ~CudaDevice() = default;
};

#endif // RT_CUDA_DEVICE_H
