#ifndef RT_TT_LIBRARY_H
#define RT_TT_LIBRARY_H

#include <array>
#include <string>
#include <vector>

#include <nexus-api.h>

#include "tenstorrent.h"

class TTRuntime;
class TTLibrary;

class TTKernel {
  TTLibrary *library;
 public:
  TTKernel(TTLibrary *_l) : library(_l) {}
  TTLibrary *getLibrary() const { return library; }
};

class TTLibrary {
  TTRuntime *rt;
  std::string file;
  TTKernel kernel;
  bool loaded;

  ttm::KernelHandle reader_kernel;
  ttm::KernelHandle writer_kernel;
  ttm::KernelHandle compute_kernel;

 public:
  TTLibrary(TTRuntime *rt = nullptr, const std::string &filename = "",
             nxs_uint library_settings = 0) : rt(rt), file(filename), kernel(this), loaded(false) {
  }
  TTLibrary(const TTLibrary &other) = default;

  ~TTLibrary() = default;

  TTKernel *getKernel() { return &kernel; }

  typedef std::vector<uint32_t> CompileTimeArgs;
  typedef std::array<uint32_t, NXS_KERNEL_MAX_ARGS> RunTimeArgs;

  void jitProgram(ttm::Program &program, const ttm::CoreRange &cores, const CompileTimeArgs &compile_time_args);
  void setupCoreRuntime(ttm::Program &program, const ttm::CoreCoord &core, const RunTimeArgs &run_time_args);
};

#endif  // RT_TT_LIBRARY_H