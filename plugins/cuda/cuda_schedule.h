#ifndef RT_CUDA_SCHEDULE_H
#define RT_CUDA_SCHEDULE_H

#include <cuda_command.h>
#include <cuda_utils.h>

#include <string>
#include <vector>

class CudaRuntime;

using namespace nxs;

class CudaSchedule {
  typedef std::vector<CudaCommand *> Commands;

  nxs_int device_id;
  nxs_uint settings;
  float time_ms;
  Commands commands;

 public:
  nxs_int getDeviceId() const { return device_id; }

  CudaSchedule(nxs_int dev_id = -1, nxs_uint settings = 0)
      : device_id(dev_id), settings(settings), time_ms(0) {
    commands.reserve(8);
  }
  ~CudaSchedule() { commands.clear(); }

  void addCommand(CudaCommand *command) {
    commands.push_back(command);
  }

  float getTime() const { return time_ms; }

  Commands getCommands() {
  return commands;
  }

  nxs_status run(cudaStream_t stream) {
    CUevent start_event, end_event;
    if (settings & NXS_EventSettings_Timing) {
      CUDA_CHECK(NXS_InvalidCommand, cudaEventCreate, &start_event);
      CUDA_CHECK(NXS_InvalidCommand, cudaEventCreate, &end_event);
      CUDA_CHECK(NXS_InvalidCommand, cudaEventRecord, start_event, stream);
    }

    for (auto cmd : commands) {
      auto status = cmd->runCommand(stream);
      if (!nxs_success(status)) return status;
    }
    if (settings & NXS_EventSettings_Timing) {
      CUDA_CHECK(NXS_InvalidCommand, cudaEventRecord, end_event, stream);
      CUDA_CHECK(NXS_InvalidCommand, cudaEventSynchronize, end_event);
      CUDA_CHECK(NXS_InvalidCommand, cudaEventElapsedTime, &time_ms,
                 start_event, end_event);
      NXSAPI_LOG(NXSAPI_STATUS_NOTE, "Time: " << time_ms << " ms");
      CUDA_CHECK(NXS_InvalidCommand, cudaEventDestroy, start_event);
      CUDA_CHECK(NXS_InvalidCommand, cudaEventDestroy, end_event);
    }
    return NXS_Success;
  }

  void release(CudaRuntime *rt);
};

#endif // RT_CUDA_SCHEDULE_H