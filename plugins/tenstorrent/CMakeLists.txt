#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/runtime_libs")
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

set(TT_METAL_CMAKE_PATH $ENV{TT_METAL_CMAKE_PATH} CACHE PATH "Path to which TT Metal has been installed")

message(STATUS "TT Metal path: ${TT_METAL_CMAKE_PATH}")

if(EXISTS ${TT_METAL_CMAKE_PATH})
    list(APPEND CMAKE_PREFIX_PATH "${TT_METAL_CMAKE_PATH}/lib/cmake/tt-metalium")
    list(APPEND CMAKE_PREFIX_PATH "${TT_METAL_CMAKE_PATH}/lib/cmake/fmt")
    list(APPEND CMAKE_PREFIX_PATH "${TT_METAL_CMAKE_PATH}/lib/cmake/umd")
    list(APPEND CMAKE_PREFIX_PATH "${TT_METAL_CMAKE_PATH}/lib/cmake/spdlog")
    list(APPEND CMAKE_PREFIX_PATH "${TT_METAL_CMAKE_PATH}/lib/cmake/tt-logger")
    list(APPEND CMAKE_PREFIX_PATH "${TT_METAL_CMAKE_PATH}/share/cmake/nlohmann_json")

    find_package(TT-Metalium REQUIRED)

    add_library(tt_plugin SHARED
      tt_buffer.cpp
      tt_command.cpp
      tt_library.cpp
      tt_runtime.cpp
      tt_schedule.cpp)


    target_link_libraries(tt_plugin PRIVATE TT::Metalium nxs-api)

    if(UNIX AND (NOT APPLE))
        target_link_options(tt_plugin PRIVATE "LINKER:--exclude-libs,ALL")
    endif()
    if(WIN32)
        target_link_options(tt_plugin PRIVATE "LINKER:--enable-auto-import,--enable-runtime-pseudo-reloc")
    endif()

else()

    message(STATUS "TT Metal not found!")

endif()
