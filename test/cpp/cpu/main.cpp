#include <nexus.h>

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
    auto devices = runtime.getDevices();
    if (!devices.empty()) {
      rt = runtime;
      dev0 = devices[0];
      break;
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
  std::vector<char> data(1024, 1);
  std::vector<float> vecA(1024, 1.0);
  std::vector<float> vecB(1024, 2.0);
  std::vector<float> vecResult_GPU(1024, 0.0);  // For GPU result

  size_t size = 1024 * sizeof(float);

  // std::ifstream f("kernel.so", std::ios::binary);
  // std::vector<char> soData;
  // soData.insert(soData.begin(), std::istream_iterator<char>(f),
  // std::istream_iterator<char>());

  auto nlib = dev0.createLibrary("cpu_kernel.so");

  auto kern = nlib.getKernel("add_vectors");
  std::cout << "   Kernel: " << kern.getId() << std::endl;

  auto buf0 = dev0.createBuffer(size, vecA.data());
  auto buf1 = dev0.createBuffer(size, vecB.data());
  auto buf2 = dev0.createBuffer(size, vecResult_GPU.data());

  auto sched = dev0.createSchedule();

  auto cmd = sched.createCommand(kern);
  cmd.setArgument(0, buf0);
  cmd.setArgument(1, buf1);
  cmd.setArgument(2, buf2);

  cmd.finalize(32, 1024);

  sched.run();

  buf2.copy(vecResult_GPU.data());

  int i = 0;
  for (auto v : vecResult_GPU) {
    if (v != 3.0) {
      std::cout << "Fail: result[" << i << "] = " << v << std::endl;
    }
    ++i;
  }
  std::cout << "Test PASSED" << std::endl;

  return 0;
}
