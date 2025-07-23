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

  CudaLibrary(void *library_data, nxs_uint data_size) : rt::Library(data_size) {
    loadModule(library_data);
  }
  CudaLibrary(const std::string &library_path) : rt::Library(0) {
    loadModule(library_path);
  }

  CudaKernel *createKernel(const std::string &kernelName) {
    return new CudaKernel(kernelName, module);
  }

private:
 void loadModule(void *library_data) {
   CUresult result = cuModuleLoadData(&module, library_data);
   if (result != CUDA_SUCCESS) {
     module = nullptr;
   }
 }

 void loadModule(const std::string &library_path) {
   CUresult result = cuModuleLoad(&module, library_path.c_str());
   if (result != CUDA_SUCCESS) {
     module = nullptr;
   }
 }
};

typedef std::vector<CudaLibrary> Libraries;

#endif // RT_CUDA_LIBRARY_H