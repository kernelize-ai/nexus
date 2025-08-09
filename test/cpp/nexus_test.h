#ifndef NEXUS_TESTS_H
#define NEXUS_TESTS_H

// Constants for test return values
#define SUCCESS 0
#define FAILURE 1

/**
 * Test basic kernel execution functionality
 * @param argc Number of command line arguments
 * @param argv Command line arguments: [runtime_name, kernel_file, kernel_name]
 * @return 0 on success, 1 on failure
 */
int test_basic_kernel(int argc, char **argv);

/**
 * Test multi-stream synchronization functionality
 * @param argc Number of command line arguments
 * @param argv Command line arguments: [runtime_name, kernel_file, kernel_name]
 * @return 0 on success, 1 on failure
 */
int test_multi_stream_sync(int argc, char **argv);

int test_smi(int argc, char **argv);

#endif  // NEXUS_TESTS_H