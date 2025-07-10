#ifndef RT_CUDA_LIBRARY_H
#define RT_CUDA_LIBRARY_H

#include <regex>
#include <string>
#include <vector>

#include <nexus-api.h>

#include <rt_library.h>
#include <cuda_kernel.h>

using namespace nxs;

class CudaLibrary : public rt::Library {

public:

  CUmodule module;

  CudaLibrary(void *library_data, nxs_uint data_size) 
    : rt::Library(data_size), module(loadModule(library_data)) {}

  CudaKernel *createKernel(const std::string &kernelName) {
    return new CudaKernel(kernelName, module);
  }

private:

  static CUmodule loadModule(void *library_data) {
    CUmodule module;
    CUresult result = cuModuleLoadData(&module, library_data);
    if (result != CUDA_SUCCESS) {
      throw std::runtime_error("Failed to load CUDA module");
    }
    return module;
  }

};

typedef std::vector<CudaLibrary> Libraries;

#endif // RT_CUDA_LIBRARY_H