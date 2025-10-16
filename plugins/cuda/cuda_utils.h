#ifndef RT_CUDA_UTILS_H
#define RT_CUDA_UTILS_H

#include <cstdint>
#include <iostream>
#include <string>

#include <cuda.h>
#include <cuda_runtime.h>

#define NXSAPI_LOGGING
#include <nexus-api.h>
#include <rt_utilities.h>

#define NXSAPI_LOG_MODULE "cuda_runtime"

////////////////////////////////////////////////////////////////////////////
// HIP CHECK and Print value
////////////////////////////////////////////////////////////////////////////
#define CUDA_CHECK(err_code, call, ...)                                      \
  do {                                                                       \
    NXSAPI_LOG(nexus::NXS_LOG_NOTE,                                           \
               "CUDA_CHECK ", #call, nxs::rt::print_value(__VA_ARGS__)); \
    cudaError_t err = call(__VA_ARGS__);                                     \
    if (err != cudaSuccess) {                                                \
      NXSAPI_LOG(nexus::NXS_LOG_ERROR,                                          \
                 "CUDA error: ", cudaGetErrorString(err));                 \
      return err_code;                                                       \
    }                                                                        \
  } while (0)

#define CU_CHECK(err_code, call, ...)                                      \
  do {                                                                     \
    NXSAPI_LOG(nexus::NXS_LOG_NOTE,                                         \
               "CU_CHECK ", #call, nxs::rt::print_value(__VA_ARGS__)); \
    CUresult err = call(__VA_ARGS__);                                      \
    if (err != CUDA_SUCCESS) {                                             \
      const char* errorStr;                                                \
      cuGetErrorString(err, &errorStr);                                    \
      NXSAPI_LOG(nexus::NXS_LOG_ERROR, "CU error: ", errorStr);             \
      return err_code;                                                     \
    }                                                                      \
  } while (0)

#endif  // RT_CUDA_UTILS_H
