#ifndef RT_TT_COMMAND_H
#define RT_TT_COMMAND_H

#include "tenstorrent.h"

#include <rt_command.h>
#include <tt_library.h>

class TTRuntime;

class TTCommand : public nxs::rt::Command<TTKernel *, nxs_int, nxs_int> {
  TTRuntime *rt;

 public:
  TTCommand(TTRuntime *rt = nullptr, TTKernel *kernel = nullptr,
             nxs_uint command_settings = 0)
      : Command(kernel, command_settings), rt(rt) {}

  TTCommand(TTRuntime *rt, nxs_int event, nxs_command_type type,
             nxs_int event_value = 1, nxs_uint command_settings = 0)
      : Command(event, type, event_value, command_settings), rt(rt) {}

  ~TTCommand() = default;

  nxs_status runCommand(nxs_int stream) override { assert(0); return NXS_Success; }
  nxs_status runCommand(nxs_int stream, ttmd::MeshWorkload &workload,
                        ttmd::MeshCoordinateRange &dev_range,
                        ttm::CoreRange &core_range);

  void release() override {}
};

#endif  // RT_TT_COMMAND_H