#!/bin/bash

set -e

detect_os() {
  case "$(uname -s)" in
    Darwin*)  echo "macos" ;;
    Linux*)   echo "linux" ;;
    *)        echo "unknown" ;;
  esac
}

test_editable_package() {
  local original_dir=$(pwd)

  python -m venv test_editable_env
  source test_editable_env/bin/activate
  pip install -r requirements.txt
  pip install -e .

  python3 test/pynexus/test-linux-cuda.py

  cd /tmp

  python -c "import nexus; nexus.get_runtimes(); print(nexus.__file__)"

  deactivate

  cd "$original_dir"

  rm -rf test_editable_env
}

test_release_package() {
  local original_dir=$(pwd)

  python -m build

  python -m venv test_release_env
  source test_release_env/bin/activate
  pip install -r requirements.txt
  pip install dist/*.whl

  cd /tmp

  python -c "import nexus; nexus.get_runtimes(); print(nexus.__file__)"

  deactivate

  cd "$original_dir"

  rm -rf test_release_env
}

main() {
  local original_dir=$(pwd)
  local os_type=$(detect_os)

  if [[ "$os_type" == "macos" ]]; then
    echo "Running macOS package test"
  elif [[ "$os_type" == "linux" ]]; then
    echo "Running Linux package test"
    test_editable_package
    test_release_package
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
