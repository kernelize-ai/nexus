project(nexus-test)

set(DEPS)

set(GPU_ARCH "gfx1011")

if(MACOS)

    list(APPEND DEPS ${CMAKE_CURRENT_BINARY_DIR}/kernel.air ${CMAKE_CURRENT_BINARY_DIR}/kernel.metallib)

    add_custom_command(OUTPUT kernel.air
      MAIN_DEPENDENCY kernel.metal
      COMMAND xcrun -sdk macosx metal -c ${CMAKE_CURRENT_SOURCE_DIR}/kernel.metal -o ${CMAKE_CURRENT_BINARY_DIR}/kernel.air)

    add_custom_command(OUTPUT kernel.metallib
      MAIN_DEPENDENCY kernel.air
      COMMAND xcrun -sdk macosx metallib ${CMAKE_CURRENT_BINARY_DIR}/kernel.air -o ${CMAKE_CURRENT_BINARY_DIR}/kernel.metallib)

endif()

if(LINUX)

    if(EXISTS ${HIP_CMAKE_PATH})

        list(APPEND DEPS ${CMAKE_CURRENT_BINARY_DIR}/hip_kernels.hsaco)

        add_custom_command(OUTPUT hip_kernels.hsaco
          MAIN_DEPENDENCY hip_kernels.hip
          COMMAND hipcc --genco --offload-arch=${GPU_ARCH} ${CMAKE_CURRENT_SOURCE_DIR}/hip_kernels.hip -o ${CMAKE_CURRENT_BINARY_DIR}/hip_kernels.hsaco)

    endif()

    add_executable(nexus-test-linux 
    test_linux.cpp )
    target_link_libraries(nexus-test-linux PRIVATE nexus-api)

endif()


add_executable(nexus-test main.cpp ${DEPS})
target_link_libraries(nexus-test PRIVATE nexus-api)

add_executable(test_multi_stream_sync test_multi_stream_sync.cpp ${DEPS})
target_link_libraries(test_multi_stream_sync PRIVATE nexus-api)

