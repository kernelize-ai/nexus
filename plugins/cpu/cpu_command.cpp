
#include <cpu_command.h>
#include <cpu_runtime.h>
#include <nexus/log.h>
#include <rt_buffer.h>

#include <boost/fiber/all.hpp>

/************************************************************************
 * @def _cpu_barrier
 * @brief Barrier for CPU fibers
 * @return void
 ***********************************************************************/
extern "C" void NXS_API_CALL _cpu_barrier(void *barrier) {
  boost::fibers::barrier *barrier_ptr =
      static_cast<boost::fibers::barrier *>(barrier);
  barrier_ptr->wait();
}

nxs_status CpuCommand::runCommand(nxs_int stream) {
  NXSAPI_LOG(nexus::NXS_LOG_NOTE, "runCommand ", kernel, " - ", type);

  if (getArgsCount() >= 32) {
    NXSAPI_LOG(nexus::NXS_LOG_ERROR, "Too many arguments for kernel");
    return NXS_InvalidCommand;
  }
  std::array<void *, NXS_KERNEL_MAX_ARGS> bufs;  // max 32 args
  for (size_t i = 0; i < getArgsCount(); i++) {
    bufs[i] = args[i].value;
  }

  int32_t launch_size[] = {
      static_cast<int32_t>(grid_size.x),  static_cast<int32_t>(grid_size.y),
      static_cast<int32_t>(grid_size.z),  static_cast<int32_t>(block_size.x),
      static_cast<int32_t>(block_size.y), static_cast<int32_t>(block_size.z)};
  bufs[getArgsCount()] = &launch_size;
  int coords_idx = getArgsCount() + 1;

  int32_t thread_count = rt->getNumCores();
  int32_t global_size = grid_size.x * grid_size.y * grid_size.z;

  int32_t blocks_per_thread =
      global_size / thread_count + !!(global_size % thread_count);

  unsigned char *shared_memory_ptr = nullptr;
  int32_t shared_memory_aligned_per_team = 0;
  if (shared_memory_size > 0) {
    shared_memory_aligned_per_team = (shared_memory_size + 63) & ~63u;
    shared_memory_aligned_per_team += 64 * block_size.x;
    unsigned shared_memory_aligned_total = shared_memory_aligned_per_team * thread_count;
    shared_memory_ptr = (unsigned char*)aligned_alloc(64, shared_memory_aligned_total);
    assert(shared_memory_ptr);
  }

  NXSAPI_LOG(nexus::NXS_LOG_NOTE,
             "global_size: ", global_size
                             , ", thread_count: ", thread_count
                             , ", blocks_per_thread: ", blocks_per_thread);

  boost::fibers::use_scheduling_algorithm<boost::fibers::algo::shared_work>();

  // Capture futures to wait on
  std::vector<std::future<void>> futures;
  futures.reserve(thread_count);

  for (int32_t team_id = 0; team_id < thread_count; team_id++) {
    futures.push_back(rt->getThreadPool()->enqueue([&, team_id]() {
      const int32_t block_start = blocks_per_thread * team_id;

      std::vector<boost::fibers::fiber> fibers;
      fibers.reserve(block_size.x);

      void *shared_memory_ptr_team = shared_memory_ptr + shared_memory_aligned_per_team * team_id;

      boost::fibers::barrier barrier(block_size.x);
      void *cpu_barrier = &barrier;

      // for each warp in a block
      for (nxs_uint warp_idx = 0; warp_idx < block_size.x; warp_idx++) {
        fibers.push_back(boost::fibers::fiber([&, warp_idx, block_start]() {
          auto block_end =
              std::min(block_start + blocks_per_thread, global_size);
          for (nxs_uint grid_idx = block_start; grid_idx < block_end;
               grid_idx++) {
            nxs_uint launch_id[] = {
                grid_idx % grid_size.x,
                (grid_idx % (grid_size.x * grid_size.y)) / grid_size.x,
                grid_idx / (grid_size.x * grid_size.y),
                warp_idx,
                0,
                0};
            auto gptr = [&](int p) {
              return p == coords_idx     ? launch_id
                   : p == coords_idx + 1 ? shared_memory_ptr
                   : p == coords_idx + 2 ? cpu_barrier
                                         : bufs[p];
            };
            std::invoke(kernel, gptr(0), gptr(1), gptr(2), gptr(3), gptr(4),
                        gptr(5), gptr(6), gptr(7), gptr(8), gptr(9), gptr(10),
                        gptr(11), gptr(12), gptr(13), gptr(14), gptr(15),
                        gptr(16), gptr(17), gptr(18), gptr(19), gptr(20),
                        gptr(21), gptr(22), gptr(23), gptr(24), gptr(25),
                        gptr(26), gptr(27), gptr(28), gptr(29), gptr(30),
                        gptr(31));
          }
        }));
      }
      for (auto &fiber : fibers) {
        fiber.join();
      }
    }));
  }
  for (auto &future : futures) {
    future.wait();
  }

  return NXS_Success;
}
