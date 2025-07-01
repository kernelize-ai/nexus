
if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(s390x|ppc64le)$")
  # ---[ Nexus uses cpuinfo library in the thread pool
  # ---[ But it doesn't support s390x/powerpc and thus not used on s390x/powerpc
  if(NOT TARGET cpuinfo AND USE_SYSTEM_CPUINFO)
    add_library(cpuinfo SHARED IMPORTED)
    find_library(CPUINFO_LIBRARY cpuinfo)
    if(NOT CPUINFO_LIBRARY)
      message(FATAL_ERROR "Cannot find cpuinfo")
    endif()
    message("Found cpuinfo: ${CPUINFO_LIBRARY}")
    set_target_properties(cpuinfo PROPERTIES IMPORTED_LOCATION "${CPUINFO_LIBRARY}")
  elseif(NOT TARGET cpuinfo)
    if(NOT DEFINED CPUINFO_SOURCE_DIR)
      set(CPUINFO_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../third_party/cpuinfo" CACHE STRING "cpuinfo source directory")
    endif()

    set(CPUINFO_BUILD_TOOLS OFF CACHE BOOL "")
    set(CPUINFO_BUILD_UNIT_TESTS OFF CACHE BOOL "")
    set(CPUINFO_BUILD_MOCK_TESTS OFF CACHE BOOL "")
    set(CPUINFO_BUILD_BENCHMARKS OFF CACHE BOOL "")
    set(CPUINFO_LIBRARY_TYPE "static" CACHE STRING "")
    set(CPUINFO_LOG_LEVEL "error" CACHE STRING "")
    if(MSVC)
      if(NEXUS_USE_MSVC_STATIC_RUNTIME)
        set(CPUINFO_RUNTIME_TYPE "static" CACHE STRING "")
      else()
        set(CPUINFO_RUNTIME_TYPE "shared" CACHE STRING "")
      endif()
    endif()
    add_subdirectory(
      "${CPUINFO_SOURCE_DIR}"
      "${CMAKE_CURRENT_BINARY_DIR}/cpuinfo")
    # We build static version of cpuinfo but link
    # them into a shared library for Nexus, so they need PIC.
    set_property(TARGET cpuinfo PROPERTY POSITION_INDEPENDENT_CODE ON)
  endif()
endif()

