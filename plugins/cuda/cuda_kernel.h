#ifndef RT_CUDA_KERNEL_H
#define RT_CUDA_KERNEL_H

#include <string>
#include <vector>

#include <cuda_runtime.h>
#include <cuda.h>

#include <rt_kernel.h>
#include <rt_buffer.h>

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

using namespace nxs;

class CudaKernel : public rt::Kernel {

public:

  CUfunction kernel;

  CudaKernel(const std::string &name, CUmodule module)
    : Kernel(name), kernel(nullptr) {
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