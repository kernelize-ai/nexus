#include <gtest/gtest.h>

#include <nexus_test.h>

std::vector<std::string_view> nexusArgs;

int g_argc;
char** g_argv;

// Create the NexusIntegration test fixture class
class NexusIntegration : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(NexusIntegration, BASIC_KERNEL) {
  int result = test_basic_kernel(g_argc, g_argv);
  EXPECT_EQ(result, SUCCESS);
}

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