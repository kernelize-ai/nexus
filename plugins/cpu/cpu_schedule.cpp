#define NXSAPI_LOGGING

#include "cpu_schedule.h"

#include "cpu_runtime.h"

#define NXSAPI_LOG_MODULE "cpu_runtime"

float CpuSchedule::getTime() const {
#if 0
  if (start_event && end_event) {
    float time_ms;
    CUDA_CHECK(NXS_InvalidCommand, cudaEventElapsedTime, &time_ms,
               start_event, end_event);
    return time_ms;
  }
  return 0.0f;
#endif
  return 0.0f;
}

nxs_status CpuSchedule::run(nxs_int stream, nxs_uint run_settings) {
  nxs_uint settings = getSettings() | run_settings;

#if 0
  if (settings & NXS_ExecutionSettings_Timing) {
    if (!start_event) {

    }
    if (!end_event) {
      CUDA_CHECK(NXS_InvalidCommand, cudaEventCreate, &end_event);
    }
    CUDA_CHECK(NXS_InvalidCommand, cudaEventRecord, start_event, stream);
  }
#endif

  for (auto cmd : getCommands()) {
    NXSAPI_LOG(NXSAPI_STATUS_NOTE, "runCommand " << " - " << cmd->getType());
    auto status = cmd->runCommand(stream);
    if (!nxs_success(status)) return status;
  }

#if 0
  if (settings & NXS_ExecutionSettings_Timing) {
    CUDA_CHECK(NXS_InvalidCommand, cudaEventRecord, end_event, stream);
    CUDA_CHECK(NXS_InvalidCommand, cudaEventSynchronize, end_event);
  }
#endif
  return NXS_Success;
}

nxs_status CpuSchedule::release() {
  nxs_status status = Schedule::release();
#if 0
  if (start_event) {
    CUDA_CHECK(NXS_InvalidCommand, cudaEventDestroy, start_event);
    start_event = nullptr;
  }
  if (end_event) {
    CUDA_CHECK(NXS_InvalidCommand, cudaEventDestroy, end_event);
    end_event = nullptr;
  }
#endif
  return status;
}
