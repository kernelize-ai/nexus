#include <tt_command.h>
#include <tt_runtime.h>
#include <rt_buffer.h>

#include <tt-metalium/bfloat16.hpp>

#include <algorithm>

/************************************************************************
 * @def _cpu_barrier
 * @brief Barrier for CPU fibers
 * @return void
 ***********************************************************************/
nxs_status TTCommand::runCommand(nxs_int stream, ttmd::MeshWorkload &workload,
                                 ttmd::MeshCoordinateRange &dev_range, ttm::CoreRange &cores) {
  NXSAPI_LOG(nexus::NXS_LOG_NOTE, "runCommand ", kernel, " - ", cores.start_coord.x, ",", cores.start_coord.y);

  if (getArgsCount() >= 32) {
    NXSAPI_LOG(nexus::NXS_LOG_ERROR, "Too many arguments for kernel");
    return NXS_InvalidCommand;
  }

  assert(kernel);
  auto *library = kernel->getLibrary();
  assert(library);

  ttm::Program program = ttm::CreateProgram();

  // load the compile-time args
  TTLibrary::CompileTimeArgs ctas;

  auto make_cb_config = [&](tt::CBIndex cb_index, size_t tile_count, size_t tile_size_bytes, tt::DataFormat data_format) {
    TT_NOBJ_CHECK(cb_config, ttm::CircularBufferConfig, (tile_count * tile_size_bytes), {{cb_index, data_format}});
    TT_CHECK(cb_config.set_page_size, cb_index, tile_size_bytes);
    return cb_config;
  };  

  size_t tile_size = 1024;

  for (nxs_uint i = 0; i < getNumConstants(); ++i) {
    auto &cst = consts[i];
    if (std::string("CB") == consts[i].name) {
      size_t tile_count = *(nxs_int*)consts[i].value;
      size_t tile_size_bytes = tile_size * getDataTypeSize(cst.settings);
      auto data_format = getDataFormat(consts[i].settings);
      NXSAPI_LOG(nexus::NXS_LOG_NOTE, "CB size (", i, "): ", tile_size_bytes, ", format=", data_format);
      auto cb_config = make_cb_config(static_cast<tt::CBIndex>(i), tile_count, tile_size_bytes, data_format);
      TT_CHECK(ttm::CreateCircularBuffer, program, cores, cb_config);
      ctas.push_back(i);
    } else {
      NXSAPI_LOG(nexus::NXS_LOG_ERROR, "Constant not supported: ", consts[i].name);
      //assert(0); // unsupported cta
    }
  }

  // jit the programs
  library->jitProgram(program, cores, ctas);

  // collect uniform args
  TTLibrary::RunTimeArgs rt_args;
  size_t numArgs = getArgsCount();
  assert(numArgs <= NXS_KERNEL_MAX_ARGS - 5);
  for (size_t i = 0; i < numArgs; i++) {
    uint32_t arg_val = *static_cast<uint32_t *>(args[i].value);
    NXSAPI_LOG(nexus::NXS_LOG_NOTE, "Runtime arg: ", i, "=", arg_val);
    rt_args[i] = arg_val;
  }

  // compute persistent grid size
  int total_grid_size = grid_size.x * grid_size.y * grid_size.z;
  int persistent_grid_stride = std::max(1, total_grid_size / (int)cores.size());
  NXSAPI_LOG(nexus::NXS_LOG_NOTE, "Total grid size: ", total_grid_size, ", cores: ", cores.size(), ", persistent grid stride: ", persistent_grid_stride);

  library->setupCommonRuntime(program, rt_args);

  // set params
  int persistent_grid_idx = 0;
  for (const auto& core : cores) {
    TTLibrary::RunTimeArgs core_rt_args;
    core_rt_args[0] = persistent_grid_idx * persistent_grid_stride;
    core_rt_args[1] = persistent_grid_idx * persistent_grid_stride + persistent_grid_stride;
    if (core_rt_args[1] > total_grid_size)
      core_rt_args[1] = total_grid_size;
    NXSAPI_LOG(nexus::NXS_LOG_NOTE, "Launch params: grid_idx=", persistent_grid_idx, ", start=", core_rt_args[0], ", end=", core_rt_args[1]);
    library->setupCoreRuntime(program, core, core_rt_args);
    persistent_grid_idx++;
  }

  // local or passed in?
  TT_CHECK(workload.add_program, dev_range, std::move(program));
  return NXS_Success;
}
