#include <nexus.h>

#include <fstream>
#include <iostream>
#include <numeric>

std::vector<std::string_view> nexusArgs;

int main() {
  auto sys = nexus::getSystem();
  auto rt = sys.getRuntime(0);

  auto devCount = rt.getDeviceCount();

  auto dev0 = rt.getDevice(0);

  auto nlib = dev0.createLibrary("cuda_kernels/add_vectors.ptx");
  auto kern = nlib.getKernel("add_vectors");

  //auto kernelID = dev0.loadKernel("add_vectors");

  std::cout << "Kernel ID: " << kern.getId() << std::endl;

  size_t size = 1024 * sizeof(float);

  std::vector<float> vecA(1024, 1.0);
  auto buf0 = dev0.createBuffer(size, vecA.data());
  std::cout << "Buffer 0 ID: " << buf0.getId() << std::endl;

  std::vector<float> vecB(1024, 2.0);
  auto buf1 = dev0.createBuffer(size, vecB.data());
  std::cout << "Buffer 1 ID: " << buf1.getId() << std::endl;

  std::vector<float> vecC(1024, 0.0);
  auto buf2 = dev0.createBuffer(size, vecC.data());
  std::cout << "Buffer 2 ID: " << buf2.getId() << std::endl;

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
    }
    ++i;
  }

  std::cout << std::endl << "Linux Test PASSED" << std::endl << std::endl;
  
  return 0;
}
