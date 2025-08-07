#include <gtest/gtest.h>
#include <nexus.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <numeric>

#define SUCCESS 0
#define FAILURE 1

int g_argc;
char** g_argv;

int test_multi_stream_sync(int argc, char **argv) {

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

  std::vector<float> vecA(1024, 1.0);
  std::vector<float> vecB(1024, 2.0);
  std::vector<float> vecResult_GPU(1024, 0.0);  // For GPU result

  size_t size = 1024 * sizeof(float);

  auto nlib = dev0.createLibrary(kernel_file);

  if (!nlib) {
    std::cout << "Failed to load library: " << kernel_file << std::endl;
    return FAILURE; 
  }

  auto kern = nlib.getKernel(kernel_name);
  if (!kern) {
    std::cout << "Failed to get kernel: " << kernel_name << std::endl;
    return FAILURE;
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
  sched0.run(stream0, NXS_ExecutionSettings_NonBlocking);
  sched1.run(stream1, NXS_ExecutionSettings_NonBlocking);

  evFinal.wait();

  buf3.copy(vecResult_GPU.data());

  int i = 0;
  for (auto v : vecResult_GPU) {
    if (v != 4.0) {
      std::cout << "Fail: result[" << i << "] = " << v << std::endl;
      return FAILURE;
    }
    ++i;
  }
  std::cout << "\n\n Test PASSED \n\n" << std::endl;

  return SUCCESS;
}

// Create the NexusIntegration test fixture class
class NexusIntegration : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(NexusIntegration, MULTI_STREAM_SYNC) {
  int result = test_multi_stream_sync(g_argc, g_argv);
  EXPECT_EQ(result, SUCCESS);
}

int main(int argc, char** argv) {
  g_argc = argc;
  g_argv = argv;

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}