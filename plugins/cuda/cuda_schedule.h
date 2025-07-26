#ifndef RT_CUDA_SCHEDULE_H
#define RT_CUDA_SCHEDULE_H

#include <cuda_command.h>
#include <cuda_utils.h>

#include <string>
#include <vector>

class CudaRuntime;

using namespace nxs;

class CudaSchedule {
 public:
  typedef std::vector<CudaCommand *> Commands;

  nxs_int device_id;
  Commands commands;

  CudaSchedule(nxs_int dev_id) : device_id(dev_id) {}
  ~CudaSchedule() = default;

  void addCommand(CudaCommand *command) {
    commands.push_back(command);
  }

  Commands getCommands() {
  return commands;
  }

  nxs_status run(cudaStream_t stream) {
    for (auto cmd : commands) {
      auto status = cmd->runCommand(stream);
      if (!nxs_success(status)) return status;
    }
    return NXS_Success;
  }

  void release(CudaRuntime *rt);
};

#endif // RT_CUDA_SCHEDULE_H