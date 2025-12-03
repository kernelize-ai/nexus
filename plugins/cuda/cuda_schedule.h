#ifndef RT_CUDA_SCHEDULE_H
#define RT_CUDA_SCHEDULE_H

#include <cuda_command.h>
#include <cuda_device.h>
#include <cuda_utils.h>
#include <rt_schedule.h>

class CudaSchedule : public nxs::rt::Schedule<CudaCommand, CudaDevice *, CUstream> {
  bool is_graph_built;
  cudaGraph_t graph;
  cudaGraphExec_t graphExec;
  CUevent start_event, end_event;

 public:
  CudaSchedule(CudaDevice *device = nullptr, nxs_uint settings = 0)
      : Schedule(device, settings),
        is_graph_built(false),
        start_event(nullptr),
        end_event(nullptr) {}
  virtual ~CudaSchedule() = default;

  float getTime() const;

  nxs_status run(CUstream stream, nxs_uint run_settings) override;

  nxs_status release() override;

};

#endif // RT_CUDA_SCHEDULE_H