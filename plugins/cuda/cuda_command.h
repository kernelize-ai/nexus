#ifndef RT_CUDA_COMMAND_H
#define RT_CUDA_COMMAND_H

#include <rt_command.h>
#include <cuda_library.h>
#include <cuda_buffer.h>

using namespace nxs;

class CudaCommand : public rt::Command {

public:

  CudaKernel *cudaKernel;
  Buffers buffers;

  int gridSize = -1;
  int blockSize = -1;

  CudaCommand(CudaKernel *cudaKernel) : cudaKernel(cudaKernel) {}
  ~CudaCommand() = default;

  nxs_status setArgument(nxs_int argument_index, CudaBuffer *buffer) {
    if (argument_index >= buffers.size())
      buffers.push_back(buffer);
    else
      buffers[argument_index] = buffer;
    return NXS_Success;
  }

  nxs_status finalize(nxs_int group_size, nxs_int grid_size) {
    gridSize = grid_size;
    blockSize = group_size;
    return NXS_Success;
  }
};

typedef std::vector<CudaCommand *> Commands;

#endif // RT_CUDA_COMMAND_H