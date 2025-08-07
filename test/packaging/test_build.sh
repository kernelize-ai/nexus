#!/bin/bash

set -e

detect_os() {
  case "$(uname -s)" in
    Darwin*)  printf "macos" ;;
    Linux*)   printf "linux" ;;
    *)        printf "unknown" ;;
  esac
}

BUILD_DIR=build.local

main() {
  local original_dir=$(pwd)
  local os_type=$(detect_os)

  mkdir -p $BUILD_DIR
  cd $BUILD_DIR
  cmake ..
  make -j$(nproc)

  if [[ "$os_type" == "macos" ]]; then
    printf "Running macOS test"
    #./test/cpp/gpu/nexus_gpu_integration_test metal metal_kernels/kernel.metallib add_vectors

  elif [[ "$os_type" == "linux" ]]; then
    printf "Running Linux test"
    ./test/cpp/gpu/test_basic_kernel cuda cuda_kernels/add_vectors.ptx add_vectors
    ./test/cpp/gpu/test_smi cuda cuda_kernels/add_vectors.ptx add_vectors
    ./test/cpp/gpu/test_multi_stream_sync cuda cuda_kernels/add_vectors.ptx add_vectors
    ./test/cpp/gpu/test_graph cuda cuda_kernels/add_vectors.ptx add_vectors
  else
    printf "Unsupported OS: $os_type"
    exit 1
  fi

  cd "$original_dir"
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  main "$@"
  exit 0
fi
