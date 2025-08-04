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
  float time_ms;
  nxs_uint command_settings;
  std::vector<void *> args;
  std::vector<void *> args_ref;
  nxs_long block_size;
  nxs_long grid_size;

  CudaCommand(CUfunction cudaKernel = nullptr, nxs_uint command_settings = 0)
      : cudaKernel(cudaKernel),
        type(NXS_CommandType_Dispatch),
        time_ms(0),
        args(CUDA_COMMAND_MAX_ARGS, nullptr),
        args_ref(CUDA_COMMAND_MAX_ARGS, nullptr),
        command_settings(command_settings) {
  }

  CudaCommand(CUevent event, nxs_command_type type, nxs_int event_value = 1,
              nxs_uint command_settings = 0)
      : event(event),
        type(type),
        event_value(event_value),
        command_settings(command_settings) {}

  ~CudaCommand() = default;

  float getTime() const { return time_ms; }

  nxs_status setArgument(nxs_int argument_index, nxs::rt::Buffer *buffer) {
    if (argument_index >= CUDA_COMMAND_MAX_ARGS) return NXS_InvalidArgIndex;

    args[argument_index] = buffer->get();
    args_ref[argument_index] = &args[argument_index];
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
        CUevent start_event, end_event;
        if (command_settings & NXS_EventSettings_Timing) {
          CUDA_CHECK(NXS_InvalidCommand, cudaEventCreate, &start_event);
          CUDA_CHECK(NXS_InvalidCommand, cudaEventCreate, &end_event);
          CUDA_CHECK(NXS_InvalidCommand, cudaEventRecord, start_event, stream);
        }

        int flags = 0;
        CU_CHECK(NXS_InvalidCommand, cuLaunchKernel, cudaKernel, grid_size, 1,
                 1, block_size, 1, 1, 0, stream, args_ref.data(), nullptr);
        // cuLaunchCooperativeKernel - for inter-block coordination
        // cuLaunchKernelMultiDevice - for multi-device kernels
        // cuLaunchKernelPDL - cluster level launch
        if (command_settings & NXS_EventSettings_Timing) {
          CUDA_CHECK(NXS_InvalidCommand, cudaEventRecord, end_event, stream);
          CUDA_CHECK(NXS_InvalidCommand, cudaEventSynchronize, end_event);
          CUDA_CHECK(NXS_InvalidCommand, cudaEventElapsedTime, &time_ms,
                     start_event, end_event);
          CUDA_CHECK(NXS_InvalidCommand, cudaEventDestroy, start_event);
          CUDA_CHECK(NXS_InvalidCommand, cudaEventDestroy, end_event);
        }
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