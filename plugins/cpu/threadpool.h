#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <nexus/log.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
// #include <immintrin.h>  // For AVX/SSE SIMD intrinsics
#include <random>

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <pthread.h>
#include <sched.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <mach/thread_act.h>
#include <mach/thread_policy.h>
#include <pthread.h>
#endif

class ThreadPool {
 private:
  std::vector<std::thread> workers;
  std::queue<std::function<void()>> tasks;
  std::mutex queue_mutex;
  std::condition_variable condition;
  std::atomic<bool> stop;

  void set_thread_affinity(int core_id) {
#ifdef _WIN32
    DWORD_PTR mask = 1ULL << core_id;
    SetThreadAffinityMask(GetCurrentThread(), mask);
#elif defined(__linux__)
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
#elif defined(__APPLE__)
    // macOS uses affinity tags instead of specific core binding
    thread_affinity_policy_data_t policy;
    policy.affinity_tag = core_id;

    kern_return_t ret = thread_policy_set(
        mach_thread_self(), THREAD_AFFINITY_POLICY, (thread_policy_t)&policy,
        THREAD_AFFINITY_POLICY_COUNT);

    if (ret != KERN_SUCCESS) {
      if (ret == 46) {  // KERN_POLICY_STATIC
        // Policy is static/immutable - this is common on modern macOS
        // Fall back to QoS (Quality of Service) approach
        static bool warned = false;
        if (!warned) {
          NXSAPI_LOG(
              NXSAPI_STATUS_NOTE,
              "Note: Thread affinity not available (KERN_POLICY_STATIC). Using "
              "QoS classes instead for thread scheduling hints.");
          warned = true;
        }
      } else {
        NXSAPI_LOG(NXSAPI_STATUS_ERR,
                   "Warning: thread_policy_set failed with error: " << ret);
      }
    }
    // Attempt 2: Use pthread QoS (Quality of Service) as alternative
    // This provides scheduling hints to the system
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INITIATED, 0);

    // Optional: Also set thread precedence for better scheduling
    thread_precedence_policy_data_t precedence;
    precedence.importance = 32;  // Normal importance

    thread_policy_set(mach_thread_self(), THREAD_PRECEDENCE_POLICY,
                      (thread_policy_t)&precedence,
                      THREAD_PRECEDENCE_POLICY_COUNT);
#endif
  }

 public:
  ThreadPool(size_t threads) : stop(false) {
    for (size_t i = 0; i < threads; ++i) {
      workers.emplace_back([this, i] {
        // Set CPU affinity for this worker thread
        set_thread_affinity(
            static_cast<int>(i % std::thread::hardware_concurrency()));

        NXSAPI_LOG(NXSAPI_STATUS_NOTE,
                   "Worker thread "
                       << i << " bound to CPU core "
                       << (i % std::thread::hardware_concurrency()));

        for (;;) {
          std::function<void()> task;
          {
            std::unique_lock<std::mutex> lock(this->queue_mutex);
            this->condition.wait(
                lock, [this] { return this->stop || !this->tasks.empty(); });

            if (this->stop && this->tasks.empty()) return;

            task = std::move(this->tasks.front());
            this->tasks.pop();
          }
          task();
        }
      });
    }
  }

  template <class F>
  auto enqueue(F&& f) -> std::future<void> {
    using return_type = void;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f)));

    std::future<return_type> res = task->get_future();
    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      if (stop) throw std::runtime_error("enqueue on stopped ThreadPool");

      tasks.emplace([task]() { (*task)(); });
    }
    condition.notify_one();
    return res;
  }

  ~ThreadPool() {
    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      stop = true;
    }
    condition.notify_all();
    for (std::thread& worker : workers) worker.join();
  }
};

#endif  // THREADPOOL_H