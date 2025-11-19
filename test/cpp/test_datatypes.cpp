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

int test_all_datatypes_kernel(int argc, char** argv) {
  if (argc < 4) {
    std::cout << "Usage: " << argv[0]
              << " <runtime_name> <kernel_file> <kernel_name>" << std::endl;
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

  std::cout << std::endl
            << "RUNTIME: " << runtimeName << " - " << count << std::endl
            << std::endl;

  for (int i = 0; i < count; ++i) {
    auto dev = runtime.getDevice(i);
    std::cout << "  Device: " << dev.getProp<std::string>(NP_Name) << " - "
              << dev.getProp<std::string>(NP_Architecture) << std::endl;
  }

  nexus::Device dev0 = runtime.getDevice(0);

  // Scalar values for each data type
  nxs_float valFloat = 1.5f;
  nxs_double valDouble = 2.5;
  nxs_int valInt = 10;
  nxs_short valShort = 5;
  nxs_char valChar = 3;
  nxs_uint valUInt = 100;
  nxs_long valLong = 1000LL;
  
  // Single result value
  double result_GPU = 0.0;

  // Expected result: 1.5 + 2.5 + 10 + 5 + 3 + 100 + 1000 = 1122.0
  double expected = 1.5 + 2.5 + 10.0 + 5.0 + 3.0 + 100.0 + 1000.0;

  auto nlib = dev0.createLibrary(kernel_file);

  auto kern = nlib.getKernel(kernel_name);
  if (!kern) {
    std::cout << "Failed to load kernel: " << kernel_name << std::endl;
    return FAILURE;
  }

  // Create buffer for single result value
  auto bufResult = dev0.createBuffer(sizeof(double), &result_GPU);

  auto stream0 = dev0.createStream();

  auto sched = dev0.createSchedule();

  auto cmd = sched.createCommand(kern);
  // Pass scalar values directly as arguments (8 arguments total now)
  cmd.setArgument(0, valFloat);
  cmd.setArgument(1, valDouble);
  cmd.setArgument(2, valInt);
  cmd.setArgument(3, valShort);
  cmd.setArgument(4, valChar);
  cmd.setArgument(5, valUInt);
  cmd.setArgument(6, valLong);
  cmd.setArgument(7, bufResult);

  cmd.finalize({1, 1, 1}, {1, 1, 1}, 0);  // Single thread

  sched.run(stream0, NXS_ExecutionSettings_Timing);

  auto time_ms = sched.getProp<nxs_double>(NP_ElapsedTime);
  std::cout << "Elapsed time: " << time_ms << " ms" << std::endl;

  bufResult.copy(&result_GPU);

  // Verify result
  double diff = std::abs(result_GPU - expected);
  if (diff > 1e-6) {
    std::cout << "Fail: result = " << result_GPU 
              << ", expected " << expected << ", diff = " << diff << std::endl;
    std::cout << std::endl << "Test FAILED" << std::endl << std::endl;
    return FAILURE;
  }

  std::cout << std::endl << "Test PASSED" << std::endl << std::endl;

  return SUCCESS;
}

// Create the NexusIntegration test fixture class
class NexusIntegration : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(NexusIntegration, ADD_VECTORS_ALL_DATATYPES) {
  int result = test_all_datatypes_kernel(g_argc, g_argv);
  EXPECT_EQ(result, SUCCESS);
}

int main(int argc, char** argv) {
  g_argc = argc;
  g_argv = argv;

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}