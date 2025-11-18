#ifndef RT_TT_SCHEDULE_H
#define RT_TT_SCHEDULE_H

#include <tt_command.h>
#include <rt_schedule.h>

#include <chrono>

#include "tenstorrent.h"

class TTSchedule : public nxs::rt::Schedule<TTCommand, TTDevice, nxs_int> {
  std::chrono::steady_clock::time_point start_time;
  std::chrono::steady_clock::time_point end_time;

 public:
  TTSchedule(TTDevice device = TTDevice(), nxs_uint settings = 0)
      : Schedule(device, settings) {}
  virtual ~TTSchedule() = default;

  float getTime() const;

  nxs_status run(nxs_int stream, nxs_uint run_settings) override;

  nxs_status release() override;
};

#endif  // RT_TT_SCHEDULE_H