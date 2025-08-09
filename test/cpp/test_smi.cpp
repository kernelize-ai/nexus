#include <nexus.h>

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <numeric>

#define SUCCESS 0
#define FAILURE -1

int test_smi(int argc, char **argv) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " <runtime_name>" << std::endl;
    return FAILURE;
  }

  std::string runtime_name = argv[1];

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

  std::cout << std::endl
            << "RUNTIME: " << runtimeName << " - " << count << std::endl
            << std::endl;

  for (int i = 0; i < count; ++i) {
    auto dev = runtime.getDevice(i);
    std::cout << "  Device: " << dev.getProp<std::string>(NP_Name) << " - "
              << dev.getProp<std::string>(NP_Architecture) << std::endl;
  }

  nexus::Device dev0 = runtime.getDevice(0);

  nxs_long coreUsage = dev0.getProp<nxs_long>(NP_CoreUtilization);
  if (coreUsage == FAILURE) {
    std::cout << "Failed to fetch core usage" << std::endl;
    return FAILURE;
  }

  nxs_long memUsage = dev0.getProp<nxs_long>(NP_MemoryUtilization);
  if (memUsage == FAILURE) {
    std::cout << "Failed to fetch memory usage" << std::endl;
    return FAILURE;
  }

  return SUCCESS;
}