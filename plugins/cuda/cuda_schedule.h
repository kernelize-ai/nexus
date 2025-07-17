#ifndef RT_CUDA_SCHEDULE_H
#define RT_CUDA_SCHEDULE_H

#include <string>
#include <vector>

#include <rt_schedule.h>
#include <cuda_command.h>
#include <rt_object.h>

using namespace nxs;

class CudaSchedule : public rt::Schedule {

public:

  nxs_int device_id;
  Commands commands;

  CudaSchedule(nxs_int dev_id) : device_id(dev_id) {}
  ~CudaSchedule() = default;

  void insertCommand(CudaCommand *command) {
    commands.push_back(command);
  }

  Commands getCommands() {
  return commands;
  }

};

#endif // RT_CUDA_SCHEDULE_H
