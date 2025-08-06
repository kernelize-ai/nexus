
#include "cuda_schedule.h"
#include "cuda_plugin_runtime.h"

#if 0
void CudaSchedule::release(CudaRuntime *rt) {
  for (auto cmd : commands) {
    rt->release(cmd);
  }
  commands.clear();
}
#endif