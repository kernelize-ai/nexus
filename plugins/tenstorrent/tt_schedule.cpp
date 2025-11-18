
#include "tt_schedule.h"

#include "tt_runtime.h"

float TTSchedule::getTime() const {
  return std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                               start_time)
      .count();
}

bool placeCommand(nxs_uint cmdSize, CoreRangeSet &coreRangeSet, CoreRange &cmdRange, const CoreCoord &devSize) {
  auto numRows = (cmdSize / devSize.x) + !!(cmdSize % devSize.x);
  auto tail = cmdSize % devSize.x;

  // TODO: use this instead
  // std::optional<CoreRange> select_contiguous_range_from_corerangeset(const CoreRangeSet& crs, uint32_t x, uint32_t y);
  if (numRows == 1) {
    // find gap  and return

  }
  // MUST BE A RECTANGLE
  auto nextRow = coreRangeSet.bounding_box().end_coord.y + 1;
  if (nextRow + numRows > devSize.y)
    return false;

  size_t endX = (numRows > 1 ? devSize.x : cmdSize) - 1;
  size_t endY = nextRow + numRows - 1;
  cmdRange = {{0, nextRow}, {endX, endY}};
  // ??? how to merge? this doesn't link
  //coreRangeSet = coreRangeSet.merge(cmdRange);
  return true;
}

nxs_status TTSchedule::run(nxs_int stream, nxs_uint run_settings) {
  NXSAPI_LOG(nexus::NXS_LOG_NOTE,
             "Schedule::run ");

  nxs_uint settings = getSettings() | run_settings;

  if (settings & NXS_ExecutionSettings_Timing) {
    start_time = std::chrono::steady_clock::now();
  }

  // map commands across cores
  auto device = getDevice();
  ttmd::MeshCoordinateRange device_range = ttmd::MeshCoordinateRange(device->shape());
  ttmd::MeshCommandQueue& cq = device->mesh_command_queue();
  ttmd::MeshWorkload workload;

  // get current device size
  // TODO: use CoreRangeSet to collect runs
  auto devGrid = device->logical_grid_size();

  CoreRangeSet coreRangeSet;

  for (auto cmd : getCommands()) {
    CoreRange cmdCores {{0,0}, {0,0}};
    if (!placeCommand(cmd->getGridSize(), coreRangeSet, cmdCores, devGrid)) {
      assert(0); // enqueue and start another workload
    }
    auto status = cmd->runCommand(stream, workload, device_range, cmdCores);
    if (!nxs_success(status)) return status;
  }
  ttmd::EnqueueMeshWorkload(cq, workload, false);
  ttmd::Finish(cq);


  if (settings & NXS_ExecutionSettings_Timing) {
    end_time = std::chrono::steady_clock::now();
  }
  return NXS_Success;
}

nxs_status TTSchedule::release() {
  nxs_status status = Schedule::release();
  return status;
}
