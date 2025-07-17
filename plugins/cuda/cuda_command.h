#ifndef RT_CUDA_COMMAND_H
#define RT_CUDA_COMMAND_H

#include <rt_command.h>
#include <cuda_library.h>
#include <cuda_buffer.h>
#include <cuda_schedule.h>
#include <cuda_runtime.h>
using namespace nxs;

#define NXSAPI_LOG_MODULE "cuda_runtime"

class CudaCommand : public rt::Command {

public:

  CudaKernel *cudaKernel;
  Buffers buffers;
  cudaEvent_t event;
  nxs_command_type type;
  nxs_int event_value;
  std::vector<void *> args;
  std::vector<void *> args_ref;
  nxs_long block_size;
  nxs_long grid_size;
  nxs_int n_val = 1024;
  
  CudaCommand(CudaKernel *cudaKernel) : cudaKernel(cudaKernel), type(NXS_CommandType_Dispatch) {}

  CudaCommand(cudaEvent_t event, nxs_command_type type, nxs_int event_value = 1)
    : event(event), type(type), event_value(event_value) {}

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

    for (auto& buffer : buffers)
      args.push_back(&buffer->cudaPtr);

    args.push_back(&n_val);
    return NXS_Success;
  }

  nxs_status runCommand(cudaStream_t stream) {
    NXSAPI_LOG(NXSAPI_STATUS_NOTE, "runCommand " << cudaKernel << " - " << type);
    switch (type) {
      case NXS_CommandType_Dispatch: {
        int flags = 0;
        CUresult cu_result = cuLaunchKernel(cudaKernel->kernel,
          gridSize, 1, 1, blockSize, 1, 1,
          0, stream, args.data(), nullptr);
          CHECK_CU(cu_result);
         // hipModuleLaunchCooperativeKernel - for inter-block coordination
         // hipModuleLaunchCooperativeKernelMultiDevice
         // hipLaunchKernelGGL - simplified for non-module kernels
        return NXS_Success;
      }
      case NXS_CommandType_Signal: {
        CHECK_CUDA(cudaEventRecord(event, stream));
        return NXS_Success;
      }
      case NXS_CommandType_Wait: {
        CHECK_CUDA(cudaStreamWaitEvent(stream, event, 0));
        return NXS_Success;
      }
      default:
        return NXS_InvalidCommand;
    }
    return NXS_Success;
  }

  void setDimensions(nxs_int block_size, nxs_int grid_size) {
    this->block_size = block_size;
    this->grid_size = grid_size;
  }
  void release() {}
};

typedef std::vector<CudaCommand *> Commands;

#endif // RT_CUDA_COMMAND_H