#ifndef RT_CPU_RUNTIME_H
#define RT_CPU_RUNTIME_H

#include <cpu_command.h>
#include <cpu_runtime.h>
#include <cpu_schedule.h>
#include <cpuinfo.h>
#include <rt_runtime.h>

#include <nexus/log.h>

#define NXSAPI_LOG_MODULE "cpu_runtime"

#include "threadpool.h"

using namespace nxs;

class CpuRuntime : public rt::Runtime {
  nxs_int numCores;
  ThreadPool threadpool;
  rt::Pool<rt::Buffer, 256> buffer_pool;
  rt::Pool<CpuCommand> command_pool;
  rt::Pool<CpuSchedule, 256> schedule_pool;

  nxs_int initNumCores() const {
    cpuinfo_initialize();
    return cpuinfo_get_processors_count();
  }

 public:
  CpuRuntime() : rt::Runtime(), numCores(initNumCores()), threadpool(numCores) {
    addObject((nxs_long)0);  // device 0
  }
  ~CpuRuntime() = default;

  nxs_int getNumCores() const { return numCores; }

  nxs_int getDeviceCount() const { return 1; }

  ThreadPool *getThreadPool() { return &threadpool; }

  template <typename T>
  T getPtr(nxs_int id) {
    return static_cast<T>(get(id));
  }

  rt::Buffer *getBuffer(size_t size, void *data_ptr = nullptr,
                        nxs_uint settings = 0) {
    return buffer_pool.get_new(size, data_ptr, settings);
  }
  nxs_status releaseBuffer(nxs_int buffer_id) {
    auto buf = get<rt::Buffer>(buffer_id);
    if (!buf) return NXS_InvalidBuffer;
    buffer_pool.release(buf);
    if (!dropObject(buffer_id)) return NXS_InvalidBuffer;
    return NXS_Success;
  }

  nxs_int getSchedule(nxs_int device_id, nxs_uint settings = 0) {
    auto schedule = schedule_pool.get_new(device_id, settings);
    if (!schedule) return NXS_InvalidSchedule;
    return addObject(schedule);
  }

  CpuCommand *getCommand(cpuFunction_t kernel, nxs_uint settings = 0) {
    return command_pool.get_new(this, kernel, settings);
  }

  CpuCommand *getCommand(nxs_int event, nxs_command_type type,
                         nxs_int event_value = 0, nxs_uint settings = 0) {
    return command_pool.get_new(this, event, type, event_value, settings);
  }

  nxs_status releaseCommand(nxs_int command_id) {
    auto cmd = get<CpuCommand>(command_id);
    if (!cmd) return NXS_InvalidCommand;
    // cmd->release();
    command_pool.release(cmd);
    if (!dropObject(command_id)) return NXS_InvalidCommand;
    return NXS_Success;
  }

  nxs_status releaseSchedule(nxs_int schedule_id) {
    auto sched = get<CpuSchedule>(schedule_id);
    if (!sched) return NXS_InvalidSchedule;
    for (auto cmd : sched->getCommands()) {
      // releaseCommand(cmd->getId());
    }
    sched->release();
    schedule_pool.release(sched);
    if (!dropObject(schedule_id)) return NXS_InvalidSchedule;
    return NXS_Success;
  }
};

#endif  // RT_CPU_RUNTIME_H
