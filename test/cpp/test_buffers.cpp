#include <gtest/gtest.h>
#include <nexus.h>
#include <vector>

#define SUCCESS 0
#define FAILURE 1

int g_argc;
char** g_argv;

class BufferFillTest : public ::testing::TestWithParam<size_t> {};

TEST_P(BufferFillTest, PatternSize) {
  std::string runtime_name = (g_argc > 1) ? g_argv[1] : "cuda";
  size_t pattern_size = GetParam();

  auto sys = nexus::getSystem();
  auto runtime = sys.getRuntime(runtime_name);
  ASSERT_TRUE(runtime && !runtime.getDevices().empty());
  auto dev = runtime.getDevice(0);
  size_t buffer_size = 1024;
  auto buf = dev.createBuffer(buffer_size, nullptr);

  std::vector<uint8_t> pattern(std::max(pattern_size, size_t{1}), 0);
  for (size_t i = 0; i < pattern_size; ++i)
    pattern[i] = 0xA0 + static_cast<uint8_t>(i);

  buf.fill(pattern_size == 0 ? nullptr : pattern.data(), pattern_size);

  std::vector<uint8_t> host_out(buffer_size);
  buf.copy(host_out.data(), NXS_BufferDeviceToHost);

  size_t effective_size = pattern_size == 0 ? 1 : pattern_size;
  for (size_t i = 0; i < buffer_size; ++i) {
    ASSERT_EQ(host_out[i], pattern[i % effective_size])
      << "Mismatch at byte " << i << " for pattern_size=" << pattern_size;
  }
}

INSTANTIATE_TEST_SUITE_P(AllPatternSizes, BufferFillTest,
  ::testing::Values(size_t{0}, size_t{1}, size_t{2}, size_t{4}));

int main(int argc, char** argv) {
  g_argc = argc;
  g_argv = argv;
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}