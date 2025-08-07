cmake_minimum_required(VERSION 3.18)

if(POLICY CMP0116)
# Introduced in cmake 3.20
# https://cmake.org/cmake/help/latest/policy/CMP0116.html
  cmake_policy(SET CMP0116 OLD)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")

    set(CMAKE_CXX_FLAGS_INIT "-stdlib=libc++")
    set(CMAKE_EXE_LINKER_FLAGS_INIT "-stdlib=libc++")

endif()

project(nexus CXX C)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")

    set(LINUX TRUE)
    set(ENV{PLATFORM_OS} "Linux")
    message(STATUS "Target system: Linux")

elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")

    set(MACOS TRUE)
    set(ENV{PLATFORM_OS} "MacOS")
    message(STATUS "Target system: MacOS")

endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Options
option(NEXUS_BUILD_PYTHON_MODULE "Build Python Nexus bindings" ON)
option(NEXUS_ENABLE_LOGGING "Enable Nexus Logging" ON)
option(NEXUS_BUILD_PLUGINS "Build the runtime plugins" ON)
option(NEXUS_BUILD_TESTS "Build the tests" ON)

# Ensure Python3 vars are set correctly
# used conditionally in this file and by lit tests

# Customized release build type with assertions: NEXUSRelBuildWithAsserts
if(NOT MSVC)
  set(CMAKE_C_FLAGS_NEXUSRELBUILDWITHASSERTS "-O2 -g")
  set(CMAKE_CXX_FLAGS_NEXUSRELBUILDWITHASSERTS "-O2 -g")
  set(CMAKE_C_FLAGS_NEXUSBUILDWITHO1 "-O1")
  set(CMAKE_CXX_FLAGS_NEXUSBUILDWITHO1 "-O1")
else()
  set(CMAKE_C_FLAGS_NEXUSRELBUILDWITHASSERTS "/Zi /RTC1 /bigobj /Zc:preprocessor /permissive-")
  set(CMAKE_CXX_FLAGS_NEXUSRELBUILDWITHASSERTS "/Zi /RTC1 /bigobj /Zc:preprocessor /permissive-")
  set(CMAKE_EXE_LINKER_FLAGS_NEXUSRELBUILDWITHASSERTS "/debug:fastlink /INCREMENTAL")
  set(CMAKE_MODULE_LINKER_FLAGS_NEXUSRELBUILDWITHASSERTS "/debug:fastlink /INCREMENTAL")
  set(CMAKE_SHARED_LINKER_FLAGS_NEXUSRELBUILDWITHASSERTS "/debug:fastlink /INCREMENTAL")
endif()

# Default build type
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Default build type: Debug")
  set(CMAKE_BUILD_TYPE "Debug")
endif()

if(NOT WIN32)
  find_library(TERMINFO_LIBRARY tinfo)
endif()

#if(NEXUS_BUILD_UT)
#  # This is an aggregate target for all unit tests.
#  add_custom_target(NEXUSUnitTests)
#  set_target_properties(NEXUSUnitTests PROPERTIES FOLDER "NEXUS/Tests")
#  include(AddNEXUSUnitTest)
#endif()

# Compiler flags
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_FORMAT_MACROS  -fPIC -std=gnu++17")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__STDC_FORMAT_MACROS")
endif()


# Disable warnings that show up in external code (gtest;pybind11)
if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wno-covered-switch-default -fvisibility=hidden")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4244 /wd4624 /wd4715 /wd4530")
endif()

if(NEXUS_ENABLE_LOGGING)
  add_compile_definitions(NEXUS_LOGGING)
endif()

file(COPY ${CMAKE_SOURCE_DIR}/device_lib/
     DESTINATION ${CMAKE_BINARY_DIR}/device_lib
     FILES_MATCHING PATTERN "*")

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/third_party/magic_enum/include)

# -----
add_subdirectory(third_party/json json)

# ------
# Core API source directory
add_subdirectory(src)

# ------
if(NEXUS_BUILD_PYTHON_MODULE)
  message(STATUS "Adding Python module")
  set(PYTHON_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/python/src)
  include_directories(${PYTHON_SRC_PATH})

  # Python Interpreter is used to run lit tests
  find_package(Python3 REQUIRED COMPONENTS Development.Module Interpreter)
  find_package(pybind11 CONFIG REQUIRED HINTS "${Python3_SITELIB}")

  set(NEXUS_LIBRARIES
    nexus-api

    Python3::Module
    pybind11::headers

  )

  add_library(nexus SHARED
      ${PYTHON_SRC_PATH}/main.cc
      ${PYTHON_SRC_PATH}/system.cc
  )

  # EXAMPLE_VERSION_INFO is defined by setup.py and passed into the C++ code as a
  # define (VERSION_INFO) here.
  target_compile_definitions(nexus
        PRIVATE VERSION_INFO=${EXAMPLE_VERSION_INFO})

  ## Link nexus with its dependencies
  target_link_libraries(nexus PRIVATE ${NEXUS_LIBRARIES})
  target_link_libraries(nexus PRIVATE pybind11::module)

  set_target_properties(nexus PROPERTIES
    BUILD_RPATH_USE_ORIGIN TRUE
    INSTALL_RPATH "$ORIGIN"
    BUILD_RPATH "$ORIGIN"
  )

  set_target_properties(nexus PROPERTIES PREFIX "lib" SUFFIX ".so")
  #install(TARGETS nexus DESTINATION nexus)

  if(WIN32)
    target_link_libraries(nexus PRIVATE ${CMAKE_DL_LIBS})
    set_target_properties(nexus PROPERTIES SUFFIX ".pyd")
  else()
    # Check if the platform is MacOS
    if(APPLE)
      #set(PYTHON_LDFLAGS "-undefined dynamic_lookup")
    endif()

    target_link_options(nexus PRIVATE ${PYTHON_LDFLAGS})

    #target_link_libraries(nexus PRIVATE z)
  endif()
endif()

if (UNIX AND NOT APPLE)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--exclude-libs,ALL")
endif()

if(NEXUS_BUILD_PLUGINS)
  add_subdirectory(plugins)
endif()

if(NEXUS_BUILD_TESTS)
  add_subdirectory(third_party/googletest)
  add_subdirectory(test)
endif()

