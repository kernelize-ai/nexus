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

  python -m venv build_venv
  source build_venv/bin/activate
  pip install -r requirements.txt

  rm -rf $BUILD_DIR
  mkdir -p $BUILD_DIR
  cd $BUILD_DIR
  cmake ..
  make -j$(nproc)

  printf "Running CPU tests"
  ./test/cpp/test_basic_kernel cpu kernel_libs/cpu_kernel.so add_vectors

  if [[ "$os_type" == "macos" ]]; then
    printf "Running macOS test"
    #./test/cpp/gpu/nexus_gpu_integration_test metal metal_kernels/kernel.metallib add_vectors

  elif [[ "$os_type" == "linux" ]]; then
    printf "Running Linux test"
    ./test/cpp/test_basic_kernel cuda kernel_libs/add_vectors.ptx add_vectors
    ./test/cpp/test_smi cuda kernel_libs/add_vectors.ptx add_vectors
    ./test/cpp/test_multi_stream_sync cuda kernel_libs/add_vectors.ptx add_vectors
    ./test/cpp/test_graph cuda kernel_libs/add_vectors.ptx add_vectors
    ./test/cpp/test_rotary_embedding cuda cuda_kernels/pos_encoding_kernels.ptx \
    _ZN4vllm23rotary_embedding_kernelIfLb0EEEvPKlPT_S4_PKS3_illliii
  else
    printf "Unsupported OS: $os_type"
    exit 1
  fi

  cd "$original_dir"

  deactivate

  rm -rf build_venv

}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  main "$@"
  exit 0
fi
