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

  cd "$original_dir"
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  main "$@"
  exit 0
fi
