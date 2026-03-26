import unittest

import nexus


class TestLoadLibraryMissing(unittest.TestCase):
  def test_load_library_raises_value_error_for_missing_file(self):
    runtime = nexus.get_runtimes()[0]
    device = runtime.get_devices()[0]

    with self.assertRaises(RuntimeError):
      device.load_library("missing_kernel_file.so")


if __name__ == "__main__":
  unittest.main()

