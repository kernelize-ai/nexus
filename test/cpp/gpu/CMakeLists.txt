project(nexus-test)

set(DEPS)

set(GPU_ARCH "gfx1011")

if(MACOS)
  set(KERNEL_DIR "${PROJECT_SOURCE_DIR}/kernels/metal_kernels")
  set(METALLIB_DIR "${CMAKE_BINARY_DIR}/metal_kernels")
  file(MAKE_DIRECTORY ${METALLIB_DIR})

  file(GLOB METAL_FILES "${KERNEL_DIR}/*.metal")

  foreach(METAL_FILE ${METAL_FILES})
    get_filename_component(KERNEL_NAME ${METAL_FILE} NAME_WE)

    set(AIR_FILE "${METALLIB_DIR}/${KERNEL_NAME}.air")
    set(METALLIB_FILE "${METALLIB_DIR}/${KERNEL_NAME}.metallib")

    add_custom_command(
      OUTPUT "${AIR_FILE}"
      COMMAND xcrun -sdk macosx metal -c "${METAL_FILE}" -o "${AIR_FILE}"
      DEPENDS "${METAL_FILE}"
      COMMENT "Compiling Metal shader: ${KERNEL_NAME}.metal")

    add_custom_command(
      OUTPUT "${METALLIB_FILE}"
      COMMAND xcrun -sdk macosx metallib "${AIR_FILE}" -o "${METALLIB_FILE}"
      DEPENDS "${AIR_FILE}"
      COMMENT "Creating Metal library: ${KERNEL_NAME}.metallib")

    list(APPEND METAL_TARGETS "${METALLIB_FILE}")
    list(APPEND DEPS "${METALLIB_FILE}")
  endforeach()

  if(METAL_TARGETS)
    add_custom_target(metal_libraries ALL
      DEPENDS ${METAL_TARGETS}
      COMMENT "Building all Metal libraries")
  endif()
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
endif()

file(GLOB test_files "*.cpp")
foreach(file ${test_files})
  get_filename_component(test_name ${file} NAME_WE)
  add_nexus_it(NAME ${test_name}
    SRCS ${file}
    LIBS nexus-api)
endforeach()
