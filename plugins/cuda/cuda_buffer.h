#ifndef RT_CUDA_BUFFER_H
#define RT_CUDA_BUFFER_H

#include <nexus-api.h>

#include <cstring>

#include <rt_buffer.h>

using namespace nxs;

class CudaBuffer : public rt::Buffer {

public:

  float *cudaPtr = nullptr;

  CudaBuffer(rt::Object *parent, int deviceID, size_t size,
             void *host_ptr = nullptr, bool is_owned = false)
      : Buffer(size, host_ptr, is_owned) {
    CHECK_CUDA(cudaSetDevice(deviceID));
    CHECK_CUDA(cudaMalloc(&cudaPtr, size));
    if (host_ptr != nullptr)
      CHECK_CUDA(cudaMemcpy(cudaPtr, host_ptr, size, cudaMemcpyHostToDevice));
  }
  ~CudaBuffer() {}
};

typedef std::vector<CudaBuffer *> Buffers;

#endif  // RT_CUDA_BUFFER_H