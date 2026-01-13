
#include "tt_schedule.h"

#include "tt_runtime.h"

float TTSchedule::getTime() const {
  return std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                               start_time)
      .count();
}

bool placeCommand(nxs_uint cmdSize, ttm::CoreRange &cmdRange, ttm::CoreRange &devRange, size_t rowLen) {
  auto numRows = (cmdSize / rowLen) + !!(cmdSize % rowLen);
  auto tail = cmdSize % rowLen;

  if (devRange.end_coord.y <= devRange.start_coord.y) {
    // No rows available
    return false;
  }

  // TODO: use this instead
  if (numRows == 1) {
    // find gap  and return
  } else if (numRows > devRange.end_coord.y - devRange.start_coord.y + 1) {
    numRows = devRange.end_coord.y - devRange.start_coord.y + 1;
  }

  // Compute range and make persistent if necessary
  cmdRange.start_coord.x = devRange.start_coord.x;
  cmdRange.start_coord.y = devRange.start_coord.y;
  cmdRange.end_coord.x = numRows > 1 ? devRange.end_coord.x : devRange.start_coord.x + tail - 1;
  cmdRange.end_coord.y = devRange.start_coord.y + numRows - 1;
  devRange.start_coord.y += numRows;
  NXSAPI_LOG(nexus::NXS_LOG_NOTE, "placeCommand: devRange=", devRange.start_coord.x, ",", devRange.start_coord.y, " - ", devRange.end_coord.x, ",", devRange.end_coord.y, ", numRows=", numRows);
  NXSAPI_LOG(nexus::NXS_LOG_NOTE, "placeCommand: cmdRange=", cmdRange.start_coord.x, ",", cmdRange.start_coord.y, " - ", cmdRange.end_coord.x, ",", cmdRange.end_coord.y);
  return true;
  // TODO: this fails
  //auto crange = select_contiguous_range_from_corerangeset(coreRangeSet, devSize.x, numRows);
  //if (crange) {
  //  NXSAPI_LOG(nexus::NXS_LOG_NOTE, "placeCommand: crange=", crange->start_coord.x, ",", crange->start_coord.y, " - ", crange->end_coord.x, ",", crange->end_coord.y);
  //  cmdRange = *crange;
  //  //coreRangeSet = coreRangeSet.merge(cmdRange);
  //  return true;
  //}
  return false;
}

nxs_status TTSchedule::run(nxs_int stream, nxs_uint run_settings) {
  NXSAPI_LOG(nexus::NXS_LOG_NOTE,
             "Schedule::run ");

  nxs_uint settings = getSettings() | run_settings;

  // map commands across cores
  auto *device = getDevice();
  auto device_range = device->getRange();
  auto &cq = device->getCQ();
  ttmd::MeshWorkload workload;

  // get current device size
  TT_NOBJ_CHECK(devGrid, device->get()->compute_with_storage_grid_size);
  NXSAPI_LOG(nexus::NXS_LOG_NOTE, "Device grid: ", devGrid.x, ",", devGrid.y);

//  TODO: use split_work_to_cores utility function to distribute commands across cores
//  NXSAPI_LOG(nexus::NXS_LOG_NOTE, "Core range set: ", coreRangeSet.bounding_box().start_coord.x, ",", coreRangeSet.bounding_box().start_coord.y, " - ", coreRangeSet.bounding_box().end_coord.x, ",", coreRangeSet.bounding_box().end_coord.y);

  ttm::CoreRange devRange = {{0,0}, {devGrid.x - 1, devGrid.y - 1}};
  for (auto cmd : getCommands()) {
    ttm::CoreRange cmdCores {{0,0}, {0,0}};
    if (!placeCommand(cmd->getGridSize(), cmdCores, devRange, devGrid.x)) {
      //assert(0); // enqueue and start another workload
    }
    NXSAPI_LOG(nexus::NXS_LOG_NOTE, "placeCommand: cmdCores=", cmdCores.start_coord.x, ",", cmdCores.start_coord.y, " - ", cmdCores.end_coord.x, ",", cmdCores.end_coord.y);
    auto status = cmd->runCommand(stream, workload, device_range, cmdCores);
    if (!nxs_success(status)) return status;
  }

  if (settings & NXS_ExecutionSettings_Timing) {
    start_time = std::chrono::steady_clock::now();
  }

  TT_CHECK(ttmd::EnqueueMeshWorkload, cq, workload, false);
  TT_CHECK(ttmd::Finish, cq);


  if (settings & NXS_ExecutionSettings_Timing) {
    end_time = std::chrono::steady_clock::now();
  }
  return NXS_Success;
}

nxs_status TTSchedule::release() {
  nxs_status status = Schedule::release();
  return status;
}
