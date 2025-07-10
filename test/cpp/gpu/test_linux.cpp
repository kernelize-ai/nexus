#include <nexus.h>

#include <fstream>
#include <iostream>
#include <numeric>

int main() {
  auto sys = nexus::getSystem();
  auto rt = sys.getRuntime(0);

  auto devCount = rt.getProperty(NP_Size);

  int numDevices = 0;

  if (devCount.has_value()) {
    nexus::Property prop = devCount.value();
    numDevices = devCount->getValue<long>();
  }

  if (!rt || !numDevices) {
    std::cout << "No runtime with valid devices found" << std::endl;
    return 1;
  }

  std::cout << std::endl << "Runtime loaded and found " << numDevices <<" devices" << std::endl << std::endl;

  auto dev0 = rt.getDevice(0);

  auto nlib = dev0.createLibrary("cuda_kernels/add_vectors.ptx");
  auto kern = nlib.getKernel("add_vectors");
  if (!kern) return -1;

  size_t size = 1024 * sizeof(float);

  std::vector<float> vecA(1024, 1.0);
  auto buf0 = dev0.createBuffer(size, vecA.data());

  std::vector<float> vecB(1024, 2.0);
  auto buf1 = dev0.createBuffer(size, vecB.data());

  std::vector<float> vecC(1024, 0.0);
  auto buf2 = dev0.createBuffer(size, vecC.data());

  auto sched = dev0.createSchedule();

  auto cmd = sched.createCommand(kern);

  cmd.setArgument(0, buf0); 
  cmd.setArgument(1, buf1);
  cmd.setArgument(2, buf2);

  cmd.finalize(32, 1024);

  sched.run();

  std::vector<float> vecResult_GPU(1024, 0.0); // For GPU result
  buf2.copy(vecResult_GPU.data());

  int i = 0;
  for (auto v : vecResult_GPU) {
    if (v != 3.0) {
      std::cout << "Fail: result[" << i << "] = " << v << std::endl;
      return -1;
    }
    ++i;
  }

  std::cout << std::endl << "Linux Test PASSED" << std::endl << std::endl;

  return 0;
}
