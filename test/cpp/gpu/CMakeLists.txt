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

  if(UNIX AND (NOT APPLE))
    find_package(CUDAToolkit REQUIRED)
    target_link_libraries(cuda_plugin CUDA::cudart CUDA::cuda_driver)

    target_compile_options(cuda_plugin PRIVATE -fvisibility=default)
    target_link_options(cuda_plugin PRIVATE "LINKER:--exclude-libs,ALL")
  endif()
  if(WIN32)
    target_link_options(cuda_plugin PRIVATE "LINKER:--enable-auto-import,--enable-runtime-pseudo-reloc")
  endif()

  # Compile Cuda kernels

  # Auto-detect GPU architecture
  execute_process(
    COMMAND nvidia-smi --query-gpu=compute_cap --format=csv,noheader,nounits --id=0
    OUTPUT_VARIABLE GPU_ARCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  string(REPLACE "." "" GPU_ARCH ${GPU_ARCH})

  set(KERNEL_DIR "${PROJECT_SOURCE_DIR}/kernels/cuda_kernels")
  set(PTX_DIR "${CMAKE_BINARY_DIR}/cuda_kernels")
  file(MAKE_DIRECTORY ${PTX_DIR})

  file(GLOB CU_FILES "${KERNEL_DIR}/*.cu")
  foreach(CU_FILE ${CU_FILES})
    get_filename_component(KERNEL_NAME ${CU_FILE} NAME_WE)
    set(PTX_FILE "${PTX_DIR}/${KERNEL_NAME}.ptx")

    add_custom_command(
      OUTPUT "${PTX_FILE}"
      COMMAND ${CUDAToolkit_NVCC_EXECUTABLE} -ptx -arch sm_${GPU_ARCH} -o "${PTX_FILE}" "${CU_FILE}"
      DEPENDS "${CU_FILE}")

      list(APPEND PTX_TARGETS "${PTX_FILE}")
  endforeach()

  add_custom_target(cuda_ptx ALL DEPENDS ${PTX_TARGETS})

endif()


add_executable(test_basic_kernel test_basic_kernel.cpp ${DEPS})
target_link_libraries(test_basic_kernel PRIVATE nexus-api)

add_executable(test_multi_stream_sync test_multi_stream_sync.cpp ${DEPS})
target_link_libraries(test_multi_stream_sync PRIVATE nexus-api)

