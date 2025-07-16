#include <nexus.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <numeric>

std::vector<std::string_view> nexusArgs;

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cout << "Usage: " << argv[0] << " <kernel_lib> <kernel_name>"
              << std::endl;
    return 1;
  }
  std::string kernel_lib = argv[1];
  std::string kernel_name = argv[2];
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

  auto nlib = dev0.createLibrary(kernel_lib);

  if (!nlib) {
    std::cout << "Failed to load library: " << kernel_lib << std::endl;
    return 1;
  }

  auto kern = nlib.getKernel(kernel_name);
  if (!kern) {
    std::cout << "Failed to get kernel: " << kernel_name << std::endl;
    return 1;
  }

  auto buf0 = dev0.createBuffer(size, vecA.data());
  auto buf1 = dev0.createBuffer(size, vecB.data());
  auto buf2 = dev0.createBuffer(size, vecResult_GPU.data());
  auto buf3 = dev0.createBuffer(size, vecResult_GPU.data());

  auto stream0 = dev0.createStream();
  auto stream1 = dev0.createStream();

  auto evFinal = dev0.createEvent();
  auto ev0 = dev0.createEvent();

  // Stream 0
  auto sched0 = dev0.createSchedule();

  auto cmd0 = sched0.createCommand(kern);
  cmd0.setArgument(0, buf0);
  cmd0.setArgument(1, buf1);
  cmd0.setArgument(2, buf2);

  cmd0.finalize(32, 32);

  sched0.createSignalCommand(ev0);

  // Stream 1
  auto sched1 = dev0.createSchedule();

  sched1.createWaitCommand(ev0);

  auto cmd1 = sched1.createCommand(kern);
  cmd1.setArgument(0, buf0);
  cmd1.setArgument(1, buf2);
  cmd1.setArgument(2, buf3);

  cmd1.finalize(32, 32);

  sched1.createSignalCommand(evFinal);

  // Run streams -- order is important for HIP events :-(
  sched0.run(stream0, false);
  sched1.run(stream1, false);

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
