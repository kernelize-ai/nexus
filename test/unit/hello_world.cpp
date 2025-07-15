#include <gtest/gtest.h>

class HelloWorld : public ::testing::Test {
 protected:
  // You can do set-up work for each test here.
  void SetUp() override {
    // Code here will be called immediately after the constructor (right
    // before each test).
  }

  // You can do clean-up work that doesn't throw exceptions here.
  void TearDown() override {
    // Code here will be called immediately after each test (right
    // before the destructor).
  }
};

TEST_F(HelloWorld, BasicTest) {
  // This is a basic test to ensure the test framework is working.
  EXPECT_EQ(1, 1);
}

TEST_F(HelloWorld, DISABLED_BasicBrokenTest) {
  // This is a basic test to ensure the test framework is working.
  EXPECT_EQ(1, 2);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
