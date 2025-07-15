#!/bin/bash

set -e

detect_os() {
  case "$(uname -s)" in
    Darwin*)  echo "macos" ;;
    Linux*)   echo "linux" ;;
    *)        echo "unknown" ;;
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
    echo "Running macOS build"
  elif [[ "$os_type" == "linux" ]]; then
    echo "Running Linux build"
    ./test/cpp/gpu/nexus-test-linux
  else
    echo "Unsupported OS: $os_type"
    exit 1
  fi

  cd "$original_dir"
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  main "$@"
  exit 0
fi
