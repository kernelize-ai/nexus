#ifndef RT_CUDA_UTILS_H
#define RT_CUDA_UTILS_H

#include <cstdint>
#include <iostream>
#include <string>

#include <cuda.h>
#include <cuda_runtime.h>

#define NXSAPI_LOGGING
#include <nexus-api.h>

#define NXSAPI_LOG_MODULE "cuda_runtime"

////////////////////////////////////////////////////////////////////////////
// HIP CHECK and Print value
////////////////////////////////////////////////////////////////////////////
template <typename T>
std::string print_value(T value) {
  return " - 0x" + std::to_string(reinterpret_cast<uintptr_t>((nxs_ulong)value));
}

std::string print_value() { return ""; }

template <typename T, typename... Args>
std::string print_value(T value, Args... args) {
  return print_value(value) + print_value(args...);
}

#define CUDA_CHECK(err_code, call, ...)                             \
  do {                                                              \
    NXSAPI_LOG(NXSAPI_STATUS_NOTE,                                  \
               "CUDA_CHECK " << #call << print_value(__VA_ARGS__)); \
    cudaError_t err = call(__VA_ARGS__);                            \
    if (err != cudaSuccess) {                                       \
      NXSAPI_LOG(NXSAPI_STATUS_ERR,                                 \
                 "CUDA error: " << cudaGetErrorString(err));        \
      return err_code;                                              \
    }                                                               \
  } while (0)

#define CU_CHECK(err_code, call, ...)                             \
  do {                                                            \
    NXSAPI_LOG(NXSAPI_STATUS_NOTE,                                \
               "CU_CHECK " << #call << print_value(__VA_ARGS__)); \
    CUresult err = call(__VA_ARGS__);                             \
    if (err != CUDA_SUCCESS) {                                    \
      const char* errorStr;                                       \
      cuGetErrorString(err, &errorStr);                           \
      NXSAPI_LOG(NXSAPI_STATUS_ERR, "CU error: " << errorStr);    \
      return err_code;                                            \
    }                                                             \
  } while (0)

#endif  // RT_CUDA_UTILS_H
