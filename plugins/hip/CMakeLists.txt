set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/runtime_libs")

if(NOT DEFINED HIP_CMAKE_PATH)
    if(NOT DEFINED ENV{HIP_CMAKE_PATH})
        set(HIP_CMAKE_PATH "/opt/rocm/lib/cmake" CACHE PATH "Path to which HIP has been installed")
    else()
        set(HIP_CMAKE_PATH $ENV{HIP_CMAKE_PATH} CACHE PATH "Path to which HIP has been installed")
    endif()
endif()

message(STATUS "HIP: ${HIP_CMAKE_PATH}")

if(EXISTS ${HIP_CMAKE_PATH})
    list(APPEND CMAKE_PREFIX_PATH ${HIP_CMAKE_PATH})

    find_package(hip REQUIRED)

    include_directories("/opt/rocm/include")
    link_directories("/opt/rocm/lib")

    add_library(hip_runtime SHARED hip_runtime.cpp)
    target_link_libraries(hip_runtime PRIVATE hip::host)
    target_link_options(hip_runtime PRIVATE "LINKER:-R/opt/rocm/lib")
    set_target_properties(hip_runtime PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()


