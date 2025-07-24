#ifndef RT_CUDA_KERNEL_H
#define RT_CUDA_KERNEL_H

#include <string>
#include <vector>

#include <cuda_runtime.h>
#include <cuda.h>

#include <rt_kernel.h>
#include <rt_buffer.h>

using namespace nxs;

class CudaKernel {

public:

  CUfunction kernel;

  CudaKernel(const std::string &name, CUmodule module)
    : kernel(nullptr) {
    CUresult result = cuModuleGetFunction(&kernel, module, name.c_str());
    if (result != CUDA_SUCCESS) {
      const char* error_string;
      cuGetErrorString(result, &error_string);
      kernel = nullptr;
    }
  }
  ~CudaKernel() = default;
};

typedef std::vector<CudaKernel> Kernels;

#endif // RT_CUDA_KERNEL_H