#include <tt_command.h>
#include <tt_runtime.h>
#include <rt_buffer.h>

#include <tt-metalium/bfloat16.hpp>

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

  // TODO: Get from constants?
  size_t num_tiles = 2;

  auto make_cb_config = [&](tt::CBIndex cb_index, size_t tile_size, tt::DataFormat data_format) {
    TT_NOBJ_CHECK(cb_config, ttm::CircularBufferConfig, (num_tiles * tile_size), {{cb_index, data_format}});
    TT_CHECK(cb_config.set_page_size, cb_index, tile_size);
    return cb_config;
  };  

  for (nxs_uint i = 0; i < getNumConstants(); ++i) {
    auto &cst = consts[i];
    if (std::string("CB") == consts[i].name) {
      size_t tile_size = *(nxs_int*)consts[i].value * getDataTypeSize(cst.settings);
      auto data_format = getDataFormat(consts[i].settings);
      NXSAPI_LOG(nexus::NXS_LOG_NOTE, "CB size: ", tile_size, ", format=", data_format);
      auto cb_config = make_cb_config(static_cast<tt::CBIndex>(i), tile_size, data_format);
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
  for (size_t i = 0; i < numArgs; i++) {
    uint32_t arg_val = *static_cast<uint32_t *>(args[i].value);
    NXSAPI_LOG(nexus::NXS_LOG_NOTE, "Runtime arg: ", i, "=", arg_val);
    rt_args[i] = arg_val;
  }

  // set params
  int grid_idx = 0;
  for (const auto& core : cores) {
    rt_args[numArgs] = grid_idx % grid_size.x;
    rt_args[numArgs + 1] = (grid_idx % (grid_size.x * grid_size.y)) / grid_size.x;
    rt_args[numArgs + 2] = grid_idx / (grid_size.x * grid_size.y);
    NXSAPI_LOG(nexus::NXS_LOG_NOTE, "Launch params: grid_idx=", grid_idx, ", x=", rt_args[numArgs], ", y=", rt_args[numArgs+1]);
    library->setupCoreRuntime(program, core, rt_args);
    grid_idx++;
  }

  // local or passed in?
  TT_CHECK(workload.add_program, dev_range, std::move(program));
  return NXS_Success;
}
