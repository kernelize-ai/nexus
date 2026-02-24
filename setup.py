import os
import platform
import re
import subprocess
import sys
import sysconfig
import shutil
from pathlib import Path

from setuptools import Extension, find_packages, setup
from setuptools.command.build_ext import build_ext
from setuptools.command.build_py import build_py
from setuptools.command.develop import develop
from setuptools.command.egg_info import egg_info
from setuptools.command.install import install
from setuptools.command.sdist import sdist
from distutils.command.clean import clean



from dataclasses import dataclass

import pybind11

try:
    from setuptools.command.bdist_wheel import bdist_wheel
except ImportError:
    from wheel.bdist_wheel import bdist_wheel

try:
    from setuptools.command.editable_wheel import editable_wheel
except ImportError:
    # create a dummy class, since there is no command to override
    class editable_wheel:
        pass

project_root = Path(__file__).parent
sys.path.insert(0, str(project_root))

from python.build_helpers import get_base_dir, get_cmake_dir

# Convert distutils Windows platform specifiers to CMake -A arguments
PLAT_TO_CMAKE = {
    "win32": "Win32",
    "win-amd64": "x64",
    "win-arm32": "ARM",
    "win-arm64": "ARM64",
}

def get_env_with_keys(key: list):
    for k in key:
        if k in os.environ:
            return os.environ[k]
    return ""


# Taken from https://github.com/pytorch/pytorch/blob/master/tools/setup_helpers/env.py
def check_env_flag(name: str, default: str = "") -> bool:
    return os.getenv(name, default).upper() in ["ON", "1", "YES", "TRUE", "Y"]


class CMakeClean(clean):

    def initialize_options(self):
        clean.initialize_options(self)
        self.build_temp = get_cmake_dir()

# A CMakeExtension needs a sourcedir instead of a file list.
# The name must be the _single_ output extension from the CMake build.
# If you need multiple extensions, see scikit-build.
class CMakeExtension(Extension):

    def __init__(self, name, sourcedir=".", path=""):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)
        self.path = path

class CMakeBuildPy(build_py):

    def run(self) -> None:
        self._copy_device_lib_to_package()
        self._copy_headers_to_package()

        self.run_command('build_ext')
        return super().run()

    def _copy_device_lib_to_package(self):
        source_device_lib = 'device_lib'

        nexus_package_dir = 'python/nexus'
        target_device_lib = os.path.join(nexus_package_dir, 'device_lib')

        if os.path.exists(source_device_lib):
            if os.path.exists(target_device_lib):
                shutil.rmtree(target_device_lib)
            shutil.copytree(source_device_lib, target_device_lib)
        else:
           raise RuntimeError(f"Warning: {source_device_lib} not found in repo root")

    def _copy_headers_to_package(self):
        """Copy entire include directory to package"""
        source_include = 'include'
        nexus_package_dir = 'python/nexus'
        target_include = os.path.join(nexus_package_dir, 'include')
        
        if os.path.exists(source_include):
            if os.path.exists(target_include):
                shutil.rmtree(target_include)
            shutil.copytree(source_include, target_include)
        else:
            raise RuntimeError(f"Warning: {source_include} not found in repo root")

class CMakeBuild(build_ext):
        
    user_options = build_ext.user_options + \
        [('base-dir=', None, 'base directory of Triton')]
        
    def initialize_options(self):
        build_ext.initialize_options(self)
        self.base_dir = get_base_dir()
        
    def finalize_options(self):
        build_ext.finalize_options(self)
            
    def run(self):
        try:
            out = subprocess.check_output(["cmake", "--version"])
        except OSError:
            raise RuntimeError("CMake must be installed to build the following extensions: " +
                               ", ".join(e.name for e in self.extensions))
        
        match = re.search(r"version\s*(?P<major>\d+)\.(?P<minor>\d+)([\d.]+)?", out.decode())
        cmake_major, cmake_minor = int(match.group("major")), int(match.group("minor"))
        if (cmake_major, cmake_minor) < (3, 18):
            raise RuntimeError("CMake >= 3.18.0 is required")
        
        for ext in self.extensions:
            self.build_extension(ext)
            
    def get_pybind11_cmake_args(self):
        pybind11_sys_path = get_env_with_keys(["PYBIND11_SYSPATH"])
        if pybind11_sys_path:
            pybind11_include_dir = os.path.join(pybind11_sys_path, "include")
        else:
            pybind11_include_dir = pybind11.get_include()
        return [f"-Dpybind11_INCLUDE_DIR='{pybind11_include_dir}'", f"-Dpybind11_DIR='{pybind11.get_cmake_dir()}'"]

    def build_extension(self, ext):
        ninja_dir = shutil.which('ninja')
        # lit is used by the test suite
        thirdparty_cmake_args = self.get_pybind11_cmake_args()
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        extdir = os.path.join(extdir, "nexus")
        c_dir = os.path.join(extdir, "_C")
        # create build directories
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        # python directories
        python_include_dir = sysconfig.get_path("platinclude")
        cmake_args = [
            "-G", "Ninja",  # Ninja is much faster than make
            "-DCMAKE_MAKE_PROGRAM=" +
            ninja_dir,  # Pass explicit path to ninja otherwise cmake may cache a temporary path
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
            "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=" + c_dir, "-DNEXUS_BUILD_PYTHON_MODULE=ON",
            "-DPython3_EXECUTABLE:FILEPATH=" + sys.executable, "-DPython3_INCLUDE_DIR=" + python_include_dir
        ]

        cmake_args.extend(thirdparty_cmake_args)

        # configuration
        cfg = "Debug" # get_build_type()
        build_args = ["--config", cfg]
        cmake_args += [f"-DCMAKE_BUILD_TYPE={cfg}"]
        if platform.system() == "Windows":
            cmake_args += [f"-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_{cfg.upper()}={extdir}"]
        else:
            max_jobs = os.getenv("MAX_JOBS", str(2 * os.cpu_count()))
            build_args += ['-j' + max_jobs]

        if check_env_flag("NEXUS_BUILD_WITH_CLANG_LLD"):
            cmake_args += [
                "-DCMAKE_C_COMPILER=clang",
                "-DCMAKE_CXX_COMPILER=clang++",
                "-DCMAKE_LINKER=lld",
                "-DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=lld",
                "-DCMAKE_MODULE_LINKER_FLAGS=-fuse-ld=lld",
                "-DCMAKE_SHARED_LINKER_FLAGS=-fuse-ld=lld",
            ]

        # Note that asan doesn't work with binaries that use the GPU, so this is
        # only useful for tools like triton-opt that don't run code on the GPU.
        #
        # I tried and gave up getting msan to work.  It seems that libstdc++'s
        # std::string does not play nicely with clang's msan (I didn't try
        # gcc's).  I was unable to configure clang to ignore the error, and I
        # also wasn't able to get libc++ to work, but that doesn't mean it's
        # impossible. :)
        if check_env_flag("NEXUS_BUILD_WITH_ASAN"):
            cmake_args += [
                "-DCMAKE_C_FLAGS=-fsanitize=address",
                "-DCMAKE_CXX_FLAGS=-fsanitize=address",
            ]

        env = os.environ.copy()
        cmake_dir = get_cmake_dir()
        subprocess.check_call(["cmake", ext.sourcedir] + cmake_args, cwd=cmake_dir, env=env)
        subprocess.check_call(["cmake", "--build", "."] + build_args, cwd=cmake_dir)

        runtime_libs_build = os.path.join(cmake_dir, "runtime_libs")
        runtime_libs_target = os.path.join("python", "nexus", "runtime_libs")

        if os.path.exists(runtime_libs_build):
            if os.path.exists(runtime_libs_target):
                shutil.rmtree(runtime_libs_target)

            shutil.copytree(runtime_libs_build, runtime_libs_target)

class BdistWheel(bdist_wheel):
  def finalize_options(self):
    bdist_wheel.finalize_options(self)
    # Let the platform name be auto-detected
    # Only override for Linux x86_64 builds
    if self.plat_name and 'linux' in self.plat_name and 'x86_64' in self.plat_name:
      self.plat_name = "manylinux1_x86_64"

def get_package_dirs():
    yield ("", "python")


def get_packages():
    yield from find_packages(where="python")


def get_entry_points():
    entry_points = {}
#    entry_points["openam.foo"] = [f"{b.name} = triton.backends.{b.name}" for b in backends]
    return entry_points


# Dynamically define supported Python versions and classifiers
MIN_PYTHON = (3, 9)
MAX_PYTHON = (3, 13)

PYTHON_REQUIRES = f">={MIN_PYTHON[0]}.{MIN_PYTHON[1]},<{MAX_PYTHON[0]}.{MAX_PYTHON[1] + 1}"
BASE_CLASSIFIERS = [
    "Development Status :: 4 - Beta",
    "Intended Audience :: Developers",
    "Topic :: Software Development :: Build Tools",
    "License :: OSI Approved :: MIT License",
]
PYTHON_CLASSIFIERS = [
    f"Programming Language :: Python :: {MIN_PYTHON[0]}.{m}" for m in range(MIN_PYTHON[1], MAX_PYTHON[1] + 1)
]
CLASSIFIERS = BASE_CLASSIFIERS + PYTHON_CLASSIFIERS

setup(
    name=os.environ.get("NEXUS_WHEEL_NAME", "knexus"),
    version="0.0.1003",
    author="Simon Waters, Matthew Leon, Alex Baden",
    author_email="simon@kernelize.ai",
    description="",
    long_description="",
    install_requires=[
        "setuptools>=40.8.0",
        "importlib-metadata; python_version < '3.10'",
    ],
    packages=list(get_packages()),
    package_dir=dict(get_package_dirs()),
    entry_points=get_entry_points(),
    include_package_data=True,
    data_files=[],
    package_data={'nexus': [
        'device_lib/**/*',
        'runtime_libs/**/*',
        'include/**/*',
    ]},
    ext_modules=[CMakeExtension("nexus", ".")],
    cmdclass={
        "build_ext": CMakeBuild,
        "build_py": CMakeBuildPy,
        "develop": develop,
        "clean": CMakeClean,
        "egg_info": egg_info,
        "install": install,
        "sdist": sdist,
        "bdist_wheel": BdistWheel,
    },
    zip_safe=False,
    keywords=["Runtime", "Triton"],
    url="https://github.com/kernelize-ai/nexus/",
    extras_require={"test": ["pytest>=6.0"]},
    python_requires=PYTHON_REQUIRES,
    classifiers=CLASSIFIERS,
)
