#include <gtest/gtest.h>
#include <nexus.h>
#include <vector>
#include <numeric>

#define SUCCESS 0
#define FAILURE 1

int g_argc;
char** g_argv;

// Shared fixture for all buffer tests
class BufferTest : public ::testing::Test {
 protected:
  nexus::Device dev;

  void SetUp() override {
    std::string runtime_name = (g_argc > 1) ? g_argv[1] : "cuda";
    auto sys = nexus::getSystem();
    auto runtime = sys.getRuntime(runtime_name);
    ASSERT_TRUE(runtime && !runtime.getDevices().empty());
    dev = runtime.getDevice(0);
  }
};

// --- Fill Tests (parameterized) ---

class BufferFillTest : public ::testing::TestWithParam<size_t> {
 protected:
  nexus::Device dev;

  void SetUp() override {
    std::string runtime_name = (g_argc > 1) ? g_argv[1] : "cuda";
    auto sys = nexus::getSystem();
    auto runtime = sys.getRuntime(runtime_name);
    ASSERT_TRUE(runtime && !runtime.getDevices().empty());
    dev = runtime.getDevice(0);
  }
};

TEST_P(BufferFillTest, PatternSize) {
  size_t pattern_size = GetParam();
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

// --- Shape Tests ---

TEST_F(BufferTest, CreateWithShape1D) {
  std::vector<float> data(1024, 1.0f);
  std::vector<nxs_int> shape = {1024};
  auto buf = dev.createBuffer(shape, data.data());

  auto result_shape = buf.getShape();
  ASSERT_EQ(result_shape.size(), 1);
  EXPECT_EQ(result_shape[0], 1024);
}

TEST_F(BufferTest, CreateWithMultiDimShape) {
  size_t total = 256 * 4;
  std::vector<float> data(total, 2.0f);
  std::vector<nxs_int> shape = {256, 4};
  auto buf = dev.createBuffer(shape, data.data());

  auto result_shape = buf.getShape();
  ASSERT_EQ(result_shape.size(), 2);
  EXPECT_EQ(result_shape[0], 256);
  EXPECT_EQ(result_shape[1], 4);
}

TEST_F(BufferTest, Reshape) {
  size_t vsize = 1024;
  std::vector<float> data(vsize, 1.0f);
  auto buf = dev.createBuffer(vsize * sizeof(float), data.data());

  buf.reshape({256, 4});

  auto result_shape = buf.getShape();
  ASSERT_EQ(result_shape.size(), 2);
  EXPECT_EQ(result_shape[0], 256);
  EXPECT_EQ(result_shape[1], 4);
}

TEST_F(BufferTest, MultipleReshapes) {
  size_t vsize = 1024;
  std::vector<float> data(vsize, 1.0f);
  auto buf = dev.createBuffer(vsize * sizeof(float), data.data());

  buf.reshape({512, 2});
  auto shape1 = buf.getShape();
  ASSERT_EQ(shape1.size(), 2);
  EXPECT_EQ(shape1[0], 512);
  EXPECT_EQ(shape1[1], 2);

  buf.reshape({64, 16});
  auto shape2 = buf.getShape();
  ASSERT_EQ(shape2.size(), 2);
  EXPECT_EQ(shape2[0], 64);
  EXPECT_EQ(shape2[1], 16);

  buf.reshape({4, 16, 16});
  auto shape3 = buf.getShape();
  ASSERT_EQ(shape3.size(), 3);
  EXPECT_EQ(shape3[0], 4);
  EXPECT_EQ(shape3[1], 16);
  EXPECT_EQ(shape3[2], 16);
}

TEST_F(BufferTest, ReshapePreservesData) {
  size_t vsize = 1024;
  std::vector<float> data(vsize);
  std::iota(data.begin(), data.end(), 0.0f);

  auto buf = dev.createBuffer(vsize * sizeof(float), data.data());
  buf.reshape({256, 4});

  std::vector<float> host_out(vsize);
  buf.copy(host_out.data(), NXS_BufferDeviceToHost);

  for (size_t i = 0; i < vsize; ++i) {
    ASSERT_EQ(host_out[i], static_cast<float>(i))
        << "Data mismatch at index " << i << " after reshape";
  }
}

TEST_F(BufferTest, ReshapeBackTo1D) {
  size_t vsize = 1024;
  std::vector<float> data(vsize, 3.0f);
  std::vector<nxs_int> shape = {32, 32};
  auto buf = dev.createBuffer(shape, data.data());

  buf.reshape({1024});

  auto result_shape = buf.getShape();
  ASSERT_EQ(result_shape.size(), 1);
  EXPECT_EQ(result_shape[0], 1024);
}

int main(int argc, char** argv) {
  g_argc = argc;
  g_argv = argv;
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}