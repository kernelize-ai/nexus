#ifndef RT_CUDA_COMMAND_H
#define RT_CUDA_COMMAND_H

#include <cuda_utils.h>
#include <rt_command.h>

class CudaCommand : public nxs::rt::Command<CUfunction, CUevent, CUstream> {
 public:
  CudaCommand(CUfunction kernel = nullptr, nxs_uint command_settings = 0)
      : Command(kernel, command_settings) {}

  CudaCommand(CUevent event, nxs_command_type type, nxs_int event_value = 1,
              nxs_uint command_settings = 0)
      : Command(event, type, event_value, command_settings) {}

  ~CudaCommand() = default;

  nxs_status runCommand(CUstream stream) override {
    NXSAPI_LOG(nexus::NXS_LOG_NOTE, "runCommand ", kernel, " - ", type);
    switch (type) {
      case NXS_CommandType_Dispatch: {
        CUevent start_event, end_event;
        if (settings & NXS_ExecutionSettings_Timing) {
          CUDA_CHECK(NXS_InvalidCommand, cudaEventCreate, &start_event);
          CUDA_CHECK(NXS_InvalidCommand, cudaEventCreate, &end_event);
          CUDA_CHECK(NXS_InvalidCommand, cudaEventRecord, start_event, stream);
        }

        int flags = 0;
        CU_CHECK(NXS_InvalidCommand, cuLaunchKernel, kernel, grid_size.x, grid_size.y, grid_size.z,
                 block_size.x, block_size.y, block_size.z, shared_memory_size, stream, args_ref.data(), nullptr);
        // cuLaunchCooperativeKernel - for inter-block coordination
        // cuLaunchKernelMultiDevice - for multi-device kernels
        // cuLaunchKernelPDL - cluster level launch
        if (settings & NXS_ExecutionSettings_Timing) {
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

  void release() override {}
};

#endif // RT_CUDA_COMMAND_H