#include <gtest/gtest.h>
#include <nexus.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <cmath>

#define SUCCESS 0
#define FAILURE 1

int g_argc;
char** g_argv;

// Helper function to create cos/sin cache
void create_cos_sin_cache(std::vector<float>& cos_sin_cache, int max_position, int rot_dim) {
    cos_sin_cache.resize(max_position * rot_dim);
    for (int pos = 0; pos < max_position; pos++) {
        for (int dim = 0; dim < rot_dim / 2; dim++) {
            float angle = pos / std::pow(10000.0f, 2.0f * dim / rot_dim);
            cos_sin_cache[pos * rot_dim + dim] = std::cos(angle);                    // cos part
            cos_sin_cache[pos * rot_dim + (rot_dim/2) + dim] = std::sin(angle);      // sin part
        }
    }
}

int test_rotary_embedding_kernel(int argc, char** argv) {
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

   nexus::Device dev0 = runtime.getDevice(0);

   // Test parameters (simplified case)
   const int64_t num_tokens = 4;
   const int64_t head_size = 64;
   const int64_t num_heads = 8;
   const int64_t num_kv_heads = 8;
   const int64_t max_position = 100;
   const int rot_dim = head_size;  // Simplified: rot_dim = head_size
   const bool is_neox = false;

   // Create test data
   std::vector<int64_t> positions = {0, 1, 2, 3};
   
   std::vector<float> query(num_tokens * num_heads * head_size);
   std::vector<float> key(num_tokens * num_kv_heads * head_size);
   std::vector<float> cos_sin_cache;
   
   // Initialize with random data
   std::random_device rd;
   std::mt19937 gen(rd());
   std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
   
   for (auto& val : query) val = dis(gen);
   for (auto& val : key) val = dis(gen);
   
   // Create cos/sin cache
   create_cos_sin_cache(cos_sin_cache, max_position, rot_dim);

   // Calculate sizes
   size_t positions_size = num_tokens * sizeof(int64_t);
   size_t query_size = num_tokens * num_heads * head_size * sizeof(float);
   size_t key_size = num_tokens * num_kv_heads * head_size * sizeof(float);
   size_t cache_size = max_position * rot_dim * sizeof(float);

   // Create library and kernel
   auto nlib = dev0.createLibrary(kernel_file);
   auto kern = nlib.getKernel(kernel_name);
   if (!kern) {
       std::cout << "Failed to get kernel: " << kernel_name << std::endl;
       return FAILURE;
   }

   // Create buffers
   auto buf_positions = dev0.createBuffer(positions_size, positions.data());
   auto buf_query = dev0.createBuffer(query_size, query.data());
   auto buf_key = dev0.createBuffer(key_size, key.data());
   auto buf_cos_sin_cache = dev0.createBuffer(cache_size, cos_sin_cache.data());

   auto stream0 = dev0.createStream();
   auto sched = dev0.createSchedule();
   auto cmd = sched.createCommand(kern);

   // Set kernel arguments (matching the CUDA kernel signature)
   cmd.setArgument(0, buf_positions);     // positions
   cmd.setArgument(1, buf_query);         // query
   cmd.setArgument(2, buf_key);           // key
   cmd.setArgument(3, buf_cos_sin_cache); // cos_sin_cache
   cmd.setArgument(4, rot_dim);           // rot_dim
   
   // Calculate strides (assuming contiguous layout)
   int64_t query_stride = num_heads * head_size;
   int64_t key_stride = num_kv_heads * head_size;
   int64_t head_stride = head_size;
   
   cmd.setArgument(5, query_stride);      // query_stride
   cmd.setArgument(6, key_stride);        // key_stride
   cmd.setArgument(7, head_stride);       // head_stride
   cmd.setArgument(8, (int)num_heads);    // num_heads
   cmd.setArgument(9, (int)num_kv_heads); // num_kv_heads
   cmd.setArgument(10, (int)head_size);   // head_size

   // Grid and block configuration (matching vLLM's configuration)
   nxs_uint grid_size = num_tokens;
   nxs_uint block_size = std::min(static_cast<int64_t>(num_heads * rot_dim / 2), 512L);
   
   cmd.finalize({grid_size,1,1}, {block_size,1,1});

   // Execute kernel
   sched.run(stream0);

   // Copy results back
   std::vector<float> result_query(num_tokens * num_heads * head_size);
   std::vector<float> result_key(num_tokens * num_kv_heads * head_size);
   
   buf_query.copy(result_query.data());
   buf_key.copy(result_key.data());

   std::cout << "Rotary embedding kernel completed successfully!" << std::endl;
   std::cout << "First few query values after rotation: ";
   for (int i = 0; i < 5; i++) {
       std::cout << result_query[i] << " ";
   }
   std::cout << std::endl;

std::cout << "Position[1]: " << positions[1] << std::endl;
std::cout << "Original query[" << num_heads * head_size << "]: " << query[num_heads * head_size] << std::endl;
std::cout << "Result query[" << num_heads * head_size << "]: " << result_query[num_heads * head_size] << std::endl;
std::cout << "Cache[" << rot_dim << "]: " << cos_sin_cache[rot_dim] << std::endl;  // position 1 cache
   // Basic validation - check that values changed from input
// Check if ANY values changed (not just first 10)

// Check first pair of token 1 (flat index 512/513)
int idx0 = 512, idx1 = 513;
float x = query[idx0], y = query[idx1];
float xo = result_query[idx0], yo = result_query[idx1];
float cos1 = std::cos(1.0f), sin1 = std::sin(1.0f);

// numeric checks
bool pair_ok =
    std::abs(xo - (x*cos1 - y*sin1)) < 1e-4f &&
    std::abs(yo - (y*cos1 + x*sin1)) < 1e-4f;

// length preservation across ALL pairs for token 1:
float max_len_err = 0.f;
for (int i = 0; i < head_size * num_heads; i += 2) {
  float xa = query[num_heads*head_size + i];
  float ya = query[num_heads*head_size + i + 1];
  float xb = result_query[num_heads*head_size + i];
  float yb = result_query[num_heads*head_size + i + 1];
  max_len_err = std::max(max_len_err,
      std::abs((xa*xa + ya*ya) - (xb*xb + yb*yb)));
}
std::cout << "pair_ok=" << pair_ok
          << " max_len_err=" << max_len_err << std::endl;



bool changed = false;
for (int i = 0; i < result_query.size(); i++) {
    if (std::abs(result_query[i] - query[i]) > 1e-6) {
        changed = true;
        std::cout << "Changed at index " << i << ": " << query[i] << " -> " << result_query[i] << std::endl;
        break;
    }
}
   
   if (!changed) {
       std::cout << "FAIL: Query values didn't change" << std::endl;
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

TEST_F(NexusIntegration, ROTARY_EMBEDDING_KERNEL) {
   int result = test_rotary_embedding_kernel(g_argc, g_argv);
   EXPECT_EQ(result, SUCCESS);
}

int main(int argc, char** argv) {
   g_argc = argc;
   g_argv = argv;

   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}