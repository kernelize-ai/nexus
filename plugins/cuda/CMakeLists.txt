#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/runtime_libs")
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

add_library(cuda_plugin SHARED cuda_runtime.cpp)

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

# Directories
set(KERNEL_DIR "${CMAKE_SOURCE_DIR}/plugins/cuda/kernels")
set(PTX_DIR "${PROJECT_BINARY_DIR}/cuda_kernels")
file(MAKE_DIRECTORY ${PTX_DIR})

file(GLOB CU_FILES "${KERNEL_DIR}/*.cu")
foreach(CU_FILE ${CU_FILES})
    get_filename_component(KERNEL_NAME ${CU_FILE} NAME_WE)
    set(PTX_FILE "${PTX_DIR}/${KERNEL_NAME}.ptx")
    
    add_custom_command(
        OUTPUT "${PTX_FILE}"
        COMMAND ${CUDAToolkit_NVCC_EXECUTABLE} -ptx -arch sm_${GPU_ARCH} -o "${PTX_FILE}" "${CU_FILE}"
        DEPENDS "${CU_FILE}"
    )
    list(APPEND PTX_TARGETS "${PTX_FILE}")
endforeach()

add_custom_target(cuda_ptx ALL DEPENDS ${PTX_TARGETS})
