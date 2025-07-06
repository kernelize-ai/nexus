#include <nexus.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <numeric>

std::vector<std::string_view> nexusArgs;

int main() {
  auto sys = nexus::getSystem();
  auto runtimes = sys.getRuntimes();

  // Find the first runtime with a valid device
  nexus::Runtime rt;
  nexus::Device dev0;

  for (auto runtime : runtimes) {
    auto type = runtime.getProp<std::string>(NP_Type);
    if (type == "gpu") {
      auto devices = runtime.getDevices();
      if (!devices.empty()) {
        rt = runtime;
        dev0 = devices[0];
        break;
      }
    }
  }

  if (!rt || !dev0) {
    std::cout << "No runtime with valid devices found" << std::endl;
    return 1;
  }

  auto count = rt.getDevices().size();

  std::cout << "RUNTIME: " << rt.getProp<std::string>(NP_Name) << " - " << count
            << std::endl;

  for (int i = 0; i < count; ++i) {
    auto dev = rt.getDevice(i);
    std::cout << "  Device: " << dev.getProp<std::string>(NP_Name) << " - "
              << dev.getProp<std::string>(NP_Architecture) << std::endl;
  }
  std::vector<float> vecA(1024, 1.0);
  std::vector<float> vecB(1024, 2.0);
  std::vector<float> vecResult_GPU(1024, 0.0);  // For GPU result

  size_t size = 1024 * sizeof(float);

  const char *metallib = std::getenv("NEXUS_TEST_LIB");
  if (metallib == nullptr) metallib = "kernel.metallib";

  auto nlib = dev0.createLibrary(metallib);

  auto kern = nlib.getKernel("add_vectors");
  if (!kern) return -1;

  auto buf0 = dev0.createBuffer(size, vecA.data());
  auto buf1 = dev0.createBuffer(size, vecB.data());
  auto buf2 = dev0.createBuffer(size, vecResult_GPU.data());
  auto buf3 = dev0.createBuffer(size, vecResult_GPU.data());

  auto stream0 = dev0.createStream();
  auto stream1 = dev0.createStream();

  auto evFinal = dev0.createEvent();

  // Stream 0
  auto sched0 = dev0.createSchedule();

  auto cmd0 = sched0.createCommand(kern);
  cmd0.setArgument(0, buf0);
  cmd0.setArgument(1, buf1);
  cmd0.setArgument(2, buf2);

  cmd0.finalize(32, 1024);

  auto ecmd = sched0.createSignalCommand();

  // Stream 1
  auto sched1 = dev0.createSchedule();
  sched1.createWaitCommand(ecmd.getEvent());
  auto cmd1 = sched1.createCommand(kern);
  cmd1.setArgument(0, buf0);
  cmd1.setArgument(1, buf2);
  cmd1.setArgument(2, buf3);

  cmd1.finalize(32, 1024);

  sched1.createSignalCommand(evFinal);

  // Run streams
  sched1.run(stream1, false);
  sched0.run(stream0, false);

  evFinal.wait();

  buf3.copy(vecResult_GPU.data());

  int i = 0;
  for (auto v : vecResult_GPU) {
    if (v != 4.0) {
      std::cout << "Fail: result[" << i << "] = " << v << std::endl;
    }
    ++i;
  }
  std::cout << "Test PASSED" << std::endl;

  return 0;
}
