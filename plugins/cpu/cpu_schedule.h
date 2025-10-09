#ifndef RT_CPU_SCHEDULE_H
#define RT_CPU_SCHEDULE_H

#include <cpu_command.h>
#include <rt_schedule.h>

#include <chrono>

class CpuSchedule : public nxs::rt::Schedule<CpuCommand, nxs_int> {
  std::chrono::steady_clock::time_point start_time;
  std::chrono::steady_clock::time_point end_time;

 public:
  CpuSchedule(nxs_int dev_id = -1, nxs_uint settings = 0)
      : Schedule(dev_id, settings) {}
  virtual ~CpuSchedule() = default;

  float getTime() const;

  nxs_status run(nxs_int stream, nxs_uint run_settings) override;

  nxs_status release() override;
};

#endif  // RT_CPU_SCHEDULE_H