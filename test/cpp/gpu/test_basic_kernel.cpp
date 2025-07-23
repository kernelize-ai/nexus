#include <nexus.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <numeric>

std::vector<std::string_view> nexusArgs;

#define SUCCESS 0
#define FAILURE 1

int main(int argc, char **argv) {

  if (argc < 4) {
    std::cout << "Usage: " << argv[0] << " <runtime_name> <kernel_file> <kernel_name>" << std::endl;
    return FAILURE;
  }

  std::string runtime_name = argv[1];
  std::string kernel_file = argv[2];
  std::string kernel_name = argv[3];

  auto sys = nexus::getSystem();
  auto runtime = sys.getRuntime(runtime_name);
  if (!runtime) {
    std::cout << "No runtimes found" << std::endl;
    return FAILURE;
  }

  auto devices = runtime.getDevices();
  if (devices.empty()) {
    std::cout << "No devices found" << std::endl;
    return FAILURE;
  }

  auto count = runtime.getDevices().size();

  std::string runtimeName = runtime.getProp<std::string>(NP_Name);
  
  std::cout << std::endl << "RUNTIME: " << runtimeName << " - " << count
            << std::endl << std::endl;

  for (int i = 0; i < count; ++i) {
    auto dev = runtime.getDevice(i);
    std::cout << "  Device: " << dev.getProp<std::string>(NP_Name) << " - "
              << dev.getProp<std::string>(NP_Architecture) << std::endl;
  }

  nexus::Device dev0 = runtime.getDevice(0);

  size_t vsize = 1024;
  std::vector<float> vecA(vsize, 1.0);
  std::vector<float> vecB(vsize, 2.0);
  std::vector<float> vecResult_GPU(vsize, 0.0);

  size_t size = vsize * sizeof(float);

  auto nlib = dev0.createLibrary(kernel_file);

  auto kern = nlib.getKernel(kernel_name);
  if (!kern) return FAILURE;

  auto buf0 = dev0.createBuffer(size, vecA.data());
  auto buf1 = dev0.createBuffer(size, vecB.data());
  auto buf2 = dev0.createBuffer(size, vecResult_GPU.data());

  auto sched = dev0.createSchedule();

  auto cmd = sched.createCommand(kern);
  cmd.setArgument(0, buf0);
  cmd.setArgument(1, buf1);
  cmd.setArgument(2, buf2);

  cmd.finalize(32, 32);

  sched.run();

  buf2.copy(vecResult_GPU.data());

  int i = 0;
  for (auto v : vecResult_GPU) {
    if (v != 3.0) {
      std::cout << "Fail: result[" << i << "] = " << v << std::endl;
      return FAILURE;
    }
    ++i;
  }

  std::cout << std::endl << "Test PASSED" << std::endl << std::endl;

  return SUCCESS;
}
