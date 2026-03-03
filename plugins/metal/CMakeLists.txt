#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/runtime_libs")
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

find_library(METAL Metal)
find_library(FOUNDATION Foundation)
find_library(QUARTZCORE QuartzCore)

include_directories(metal-cpp)

set(CMAKE_CXX_VISIBILITY_PRESET default)

add_library(metal_plugin SHARED metal_runtime.cpp)
target_link_libraries(metal_plugin PRIVATE
    "-framework Metal"
    "-framework Foundation"
    "-framework QuartzCore"
    nxs-api)
    
#set_target_properties(metal_plugin PROPERTIES C_VISIBILITY_PRESET)
#target_link_options(metal_plugin PRIVATE "-undefined dynamic_lookup")

if(UNIX AND (NOT APPLE))
  target_link_options(metal_plugin PRIVATE "LINKER:--exclude-libs,ALL")
endif()
if(WIN32)
  target_link_options(metal_plugin PRIVATE "LINKER:--enable-auto-import,--enable-runtime-pseudo-reloc")
endif()

