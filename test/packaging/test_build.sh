#!/bin/bash

set -e

detect_os() {
  case "$(uname -s)" in
    Darwin*)  printf "macos" ;;
    Linux*)   printf "linux" ;;
    *)        printf "unknown" ;;
  esac
}

main() {
  local original_dir=$(pwd)
  local os_type=$(detect_os)

  mkdir -p build
  cd build
  rm -rf *
  cmake ..
  make -j$(nproc)

  if [[ "$os_type" == "macos" ]]; then
    printf "Running macOS build"
  elif [[ "$os_type" == "linux" ]]; then
    printf "Running Linux build"
    ./test/cpp/gpu/test_basic_kernel cuda cuda_kernels/add_vectors.ptx add_vectors
    printf "\n\nPASSED: Basic Kernel Test\n\n"
    ./test/cpp/gpu/test_multi_stream_sync cuda cuda_kernels/add_vectors.ptx add_vectors
    printf "\n\nPASSED: Multi Stream Sync Test\n\n"
    printf "\n\nAll tests passed successfully!\n\n"
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
