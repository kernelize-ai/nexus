#ifndef RT_CPU_COMMAND_H
#define RT_CPU_COMMAND_H

#include <rt_command.h>

class CpuRuntime;

typedef void (*cpuFunction_t)(void *, void *, void *, void *, void *, void *,
                              void *, void *, void *, void *, void *, void *,
                              void *, void *, void *, void *, void *, void *,
                              void *, void *, void *, void *, void *, void *,
                              void *, void *, void *, void *, void *, void *,
                              void *, void *);

class CpuCommand : public nxs::rt::Command<cpuFunction_t, nxs_int, nxs_int> {
  CpuRuntime *rt;

 public:
  CpuCommand(CpuRuntime *rt = nullptr, cpuFunction_t kernel = nullptr,
             nxs_uint command_settings = 0)
      : Command(kernel, command_settings), rt(rt) {}

  CpuCommand(CpuRuntime *rt, nxs_int event, nxs_command_type type,
             nxs_int event_value = 1, nxs_uint command_settings = 0)
      : Command(event, type, event_value, command_settings), rt(rt) {}

  ~CpuCommand() = default;

  nxs_status runCommand(nxs_int stream) override;

  void release() override {}
};

#endif  // RT_CPU_COMMAND_H