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
