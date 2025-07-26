#ifndef RT_CUDA_COMMAND_H
#define RT_CUDA_COMMAND_H

#include <cuda_utils.h>
#include <rt_buffer.h>

#define CUDA_COMMAND_MAX_ARGS 64

class CudaCommand {

public:

  CUfunction cudaKernel;
  CUevent event;
  nxs_command_type type;
  nxs_int event_value;
  std::vector<void *> args;
  std::vector<void *> args_ref;
  nxs_long block_size;
  nxs_long grid_size;
  
  CudaCommand(CUfunction cudaKernel) : cudaKernel(cudaKernel), type(NXS_CommandType_Dispatch),
    args(CUDA_COMMAND_MAX_ARGS, nullptr), args_ref(CUDA_COMMAND_MAX_ARGS) {
    for (int i = 0; i < CUDA_COMMAND_MAX_ARGS; i++) {
      args_ref[i] = &args[i];
    }
  }

  CudaCommand(CUevent event, nxs_command_type type, nxs_int event_value = 1)
    : event(event), type(type), event_value(event_value) {}

  ~CudaCommand() = default;

  nxs_status setArgument(nxs_int argument_index, nxs::rt::Buffer *buffer) {
    if (argument_index >= CUDA_COMMAND_MAX_ARGS) return NXS_InvalidArgIndex;

    args[argument_index] = buffer->get();
    return NXS_Success;
  }

  nxs_status setScalar(nxs_int argument_index, void *value) {
    if (argument_index >= CUDA_COMMAND_MAX_ARGS)
      return NXS_InvalidArgIndex;

    args[argument_index] = value;
    args_ref[argument_index] = value;
    return NXS_Success;
  }

  nxs_status finalize(nxs_int grid_size, nxs_int block_size) {
    // TODO: check if all arguments are valid
    this->grid_size = grid_size;
    this->block_size = block_size;

    return NXS_Success;
  }

  nxs_status runCommand(CUstream stream) {
    NXSAPI_LOG(NXSAPI_STATUS_NOTE, "runCommand " << cudaKernel << " - " << type);
    switch (type) {
      case NXS_CommandType_Dispatch: {
        int flags = 0;
        CU_CHECK(NXS_InvalidCommand, cuLaunchKernel, cudaKernel, grid_size, 1,
                 1, block_size, 1, 1, 0, stream, args_ref.data(), nullptr);
        // hipModuleLaunchCooperativeKernel - for inter-block coordination
        // hipModuleLaunchCooperativeKernelMultiDevice
        // hipLaunchKernelGGL - simplified for non-module kernels
        return NXS_Success;
      }
      case NXS_CommandType_Signal: {
        CUDA_CHECK(NXS_InvalidCommand, cudaEventRecord, event, stream);
        return NXS_Success;
      }
      case NXS_CommandType_Wait: {
        CUDA_CHECK(NXS_InvalidCommand, cudaStreamWaitEvent, stream, event, 0);
        return NXS_Success;
      }
      default:
        return NXS_InvalidCommand;
    }
    return NXS_Success;
  }

  void release() {}
};

typedef std::vector<CudaCommand *> Commands;

#endif // RT_CUDA_COMMAND_H