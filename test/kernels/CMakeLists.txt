project(nexus-test)

set(CPU_TARGETS)
set(METAL_TARGETS)
set(HIP_TARGETS)
set(PTX_TARGETS)

set(KC_TARGETS)

set(KERNEL_LIBS "${CMAKE_BINARY_DIR}/kernel_libs")
file(MAKE_DIRECTORY ${KERNEL_LIBS})

### CPU Kernels - always build
file(GLOB CPU_FILES "${CMAKE_CURRENT_SOURCE_DIR}/cpu/*.c")
foreach(CPU_FILE ${CPU_FILES})
  get_filename_component(KERNEL_NAME ${CPU_FILE} NAME_WE)
  set(SO_FILE "${KERNEL_LIBS}/${KERNEL_NAME}.so")

  add_custom_command(
    OUTPUT "${SO_FILE}"
    COMMAND ${CMAKE_C_COMPILER} -shared -fPIC -o "${SO_FILE}" "${CPU_FILE}"
    DEPENDS "${CPU_FILE}"
    COMMENT "Compiling CPU kernel: ${KERNEL_NAME}.c")

  list(APPEND CPU_TARGETS ${SO_FILE})
endforeach()

if(CPU_TARGETS)
  add_custom_target(cpu_libraries ALL
    DEPENDS ${CPU_TARGETS}
    COMMENT "Building all CPU libraries")
endif()

### Metal Kernels
if(MACOS)
  set(KERNEL_DIR "${PROJECT_SOURCE_DIR}/metal_kernels")
  
  file(GLOB METAL_FILES "${KERNEL_DIR}/*.metal")

  foreach(METAL_FILE ${METAL_FILES})
    get_filename_component(KERNEL_NAME ${METAL_FILE} NAME_WE)

    set(AIR_FILE "${KERNEL_LIBS}/${KERNEL_NAME}.air")
    set(METALLIB_FILE "${KERNEL_LIBS}/${KERNEL_NAME}.metallib")

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

  ### HIP Kernels
  if(EXISTS ${HIP_CMAKE_PATH})

    execute_process(
      COMMAND rocm-smi --showhw -d 0
      OUTPUT_VARIABLE ROCM_SMI_OUTPUT
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "ROCM_SMI_OUTPUT: ${ROCM_SMI_OUTPUT}")
    string(REGEX MATCH "gfx[0-9]+" HIP_GPU_ARCH ${ROCM_SMI_OUTPUT})
    message(STATUS "HIP_GPU_ARCH: ${HIP_GPU_ARCH}")

    file(GLOB HIP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/hip_kernels/*.hip")
    foreach(HIP_FILE ${HIP_FILES})
      get_filename_component(KERNEL_NAME ${HIP_FILE} NAME_WE)
      set(HSACO_FILE "${KERNEL_LIBS}/${KERNEL_NAME}.hsaco")

      add_custom_command(
        OUTPUT "${HSACO_FILE}"
        COMMAND hipcc --genco --offload-arch=${HIP_GPU_ARCH} "${HIP_FILE}" -o "${HSACO_FILE}"
        DEPENDS "${HIP_FILE}")

        list(APPEND HIP_TARGETS "${HSACO_FILE}")
    endforeach()

    ## Build all HIP libraries
    if(HIP_TARGETS)
      add_custom_target(hip_libraries ALL
        DEPENDS ${HIP_TARGETS}
        COMMENT "Building all HIP libraries")
    endif()
  endif()

  ### CUDA Kernels
  if(CMAKE_CUDA_COMPILER)
    find_package(CUDAToolkit REQUIRED)
    # Auto-detect GPU architecture
    execute_process(
      COMMAND nvidia-smi --query-gpu=compute_cap --format=csv,noheader,nounits --id=0
      OUTPUT_VARIABLE GPU_ARCH
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "GPU_ARCH: ${GPU_ARCH}")
    if (NOT GPU_ARCH STREQUAL "")
      string(REPLACE "." "" GPU_ARCH ${GPU_ARCH})
    endif()

    file(GLOB CU_FILES "${CMAKE_CURRENT_SOURCE_DIR}/cuda_kernels/*.cu")
    foreach(CU_FILE ${CU_FILES})
      get_filename_component(KERNEL_NAME ${CU_FILE} NAME_WE)
      set(PTX_FILE "${KERNEL_LIBS}/${KERNEL_NAME}.ptx")
      set(KC_FILE "${KERNEL_LIBS}/${KERNEL_NAME}.kc")

      add_custom_command(
        OUTPUT "${PTX_FILE}"
        COMMAND ${CUDAToolkit_NVCC_EXECUTABLE} -ptx -arch sm_${GPU_ARCH} -o "${PTX_FILE}" "${CU_FILE}"
        DEPENDS "${CU_FILE}")

      list(APPEND PTX_TARGETS "${PTX_FILE}")

      add_custom_command(
        OUTPUT "${KC_FILE}"
        COMMAND python ${CMAKE_SOURCE_DIR}/tools/cuda_kc.py -a sm_${GPU_ARCH} -o "${KC_FILE}" "${CU_FILE}"
        DEPENDS "${CU_FILE}")

      list(APPEND KC_TARGETS "${KC_FILE}")
    endforeach()

    if(PTX_TARGETS)
      add_custom_target(cuda_libraries ALL
        DEPENDS ${PTX_TARGETS}
        COMMENT "Building all CUDA libraries ${PTX_TARGETS}")
    endif()
  endif()
endif()

if(KC_TARGETS)
  add_custom_target(kc_libraries ALL
    DEPENDS ${KC_TARGETS}
    COMMENT "Building all CUDA libraries ${KC_TARGETS}")
endif()