
#include "cuda_schedule.h"
#include "cuda_plugin_runtime.h"

float CudaSchedule::getTime() const {
  if (start_event && end_event) {
    float time_ms;
    CUDA_CHECK(NXS_InvalidCommand, cudaEventElapsedTime, &time_ms,
               start_event, end_event);
    return time_ms;
  }
  return 0.0f;
}

nxs_status CudaSchedule::run(CUstream stream, nxs_uint run_settings) {
  nxs_uint settings = getSettings() | run_settings;
  if (settings & NXS_ExecutionSettings_Timing) {
    if (!start_event) {
      CUDA_CHECK(NXS_InvalidCommand, cudaEventCreate, &start_event);
    }
    if (!end_event) {
      CUDA_CHECK(NXS_InvalidCommand, cudaEventCreate, &end_event);
    }
    CUDA_CHECK(NXS_InvalidCommand, cudaEventRecord, start_event, stream);
  }
  if (is_graph_built) {
    CUDA_CHECK(NXS_InvalidSchedule, cudaGraphLaunch, graphExec, stream);
  } else {
    if (settings & NXS_ExecutionSettings_Capture) {
      CUDA_CHECK(NXS_InvalidSchedule, cudaStreamBeginCapture, stream,
                 cudaStreamCaptureModeGlobal);
    }

    for (auto cmd : getCommands()) {
      auto status = cmd->runCommand(stream);
      if (!nxs_success(status)) return status;
    }
    if (settings & NXS_ExecutionSettings_Capture) {
      CUDA_CHECK(NXS_InvalidSchedule, cudaStreamEndCapture, stream, &graph);
      CUDA_CHECK(NXS_InvalidSchedule, cudaGraphInstantiate, &graphExec, graph,
                 nullptr, nullptr, 0);
      is_graph_built = true;
    }
  }
  if (settings & NXS_ExecutionSettings_Timing) {
    CUDA_CHECK(NXS_InvalidCommand, cudaEventRecord, end_event, stream);
    CUDA_CHECK(NXS_InvalidCommand, cudaEventSynchronize, end_event);
  }
  return NXS_Success;
}

nxs_status CudaSchedule::release() {
  nxs_status status = Schedule::release();
  if (start_event) {
    CUDA_CHECK(NXS_InvalidCommand, cudaEventDestroy, start_event);
    start_event = nullptr;
  }
  if (end_event) {
    CUDA_CHECK(NXS_InvalidCommand, cudaEventDestroy, end_event);
    end_event = nullptr;
  }
  if (is_graph_built) {
    cudaGraphExecDestroy(graphExec);
    cudaGraphDestroy(graph);
    is_graph_built = false;
  }
  return status;
}
