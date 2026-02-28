#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/runtime_libs")
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

# Find cpuinfo using the custom FindCPUINFO.cmake module
find_package(CPUINFO REQUIRED)

find_package(Boost REQUIRED COMPONENTS fiber context)
include_directories(${Boost_INCLUDE_DIRS})

add_library(cpu_plugin SHARED
 cpu_command.cpp
 cpu_runtime.cpp
 cpu_schedule.cpp)

 target_link_libraries(cpu_plugin PRIVATE nxs-api)
 target_link_libraries(cpu_plugin PRIVATE cpuinfo ${Boost_LIBRARIES})

if(UNIX AND (NOT APPLE))
 target_link_options(cpu_plugin PRIVATE "LINKER:--exclude-libs,ALL")
endif()
if(WIN32)
 target_link_options(cpu_plugin PRIVATE "LINKER:--enable-auto-import,--enable-runtime-pseudo-reloc")
endif()
