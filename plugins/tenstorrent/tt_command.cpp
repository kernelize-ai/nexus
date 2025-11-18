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
                                 ttmd::MeshCoordinateRange &dev_range, CoreRange &cores) {
  NXSAPI_LOG(nexus::NXS_LOG_NOTE, "runCommand ", kernel, " - ", type);

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

  size_t num_tiles = 2;

  auto make_cb_config = [&](tt::CBIndex cb_index, size_t tile_size, tt::DataFormat data_format) {
    return ttm::CircularBufferConfig(num_tiles * tile_size, {{cb_index, data_format}})
        .set_page_size(cb_index, tile_size);
  };  

  for (nxs_uint i = 0; i < getNumConstants(); ++i) {
    auto cst = consts[i];
    if (std::string("CB") == consts[i].name) {
      size_t tile_size = *(nxs_long*)consts[i].value;
      auto data_format = getDataFormat(consts[i].settings);
      ttm::CreateCircularBuffer(program, cores, make_cb_config(static_cast<tt::CBIndex>(i), tile_size, data_format));
      ctas.push_back(i);
    } else {
      assert(0); // unsupported cta
    }
  }

  // jit the programs
  library->jitProgram(program, cores, ctas);

  // collect uniform args
  TTLibrary::RunTimeArgs rt_args;
  size_t numArgs = getArgsCount();
  for (size_t i = 0; i < numArgs; i++) {
    uint32_t arg_val = *static_cast<uint32_t *>(args[i].value);
    rt_args[i] = arg_val;
  }

  // set params
  int grid_idx = 0;
  for (const auto& core : cores) {
    nxs_dim3 block_id;
    block_id.x = grid_idx % grid_size.x;
    block_id.y = grid_idx / grid_size.x;
    block_id.z = grid_idx % (grid_size.x * grid_size.y);
    rt_args[numArgs] = block_id.x;
    rt_args[numArgs + 1] = block_id.y;
    rt_args[numArgs + 2] = block_id.z;
    library->setupCoreRuntime(program, core, rt_args);
    grid_idx++;
  }

  // local or passed in?
  workload.add_program(dev_range, std::move(program));
  return NXS_Success;
}
