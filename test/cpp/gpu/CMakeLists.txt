project(nexus-test)

set(DEPS)

if(MACOS)

    list(APPEND DEPS ${CMAKE_CURRENT_BINARY_DIR}/kernel.air ${CMAKE_CURRENT_BINARY_DIR}/kernel.metallib)

    add_custom_command(OUTPUT kernel.air
      MAIN_DEPENDENCY kernel.metal
      COMMAND xcrun -sdk macosx metal -c ${CMAKE_CURRENT_SOURCE_DIR}/kernel.metal -o ${CMAKE_CURRENT_BINARY_DIR}/kernel.air)

    add_custom_command(OUTPUT kernel.metallib
      MAIN_DEPENDENCY kernel.air
      COMMAND xcrun -sdk macosx metallib ${CMAKE_CURRENT_BINARY_DIR}/kernel.air -o ${CMAKE_CURRENT_BINARY_DIR}/kernel.metallib)

    add_executable(test_multi_stream_sync test_multi_stream_sync.cpp ${DEPS})
    target_link_libraries(test_multi_stream_sync PRIVATE nexus-api)

endif()

if(LINUX)

    add_executable(nexus-test-linux 
    test_linux.cpp )
    target_link_libraries(nexus-test-linux PRIVATE nexus-api)

endif()


add_executable(nexus-test main.cpp ${DEPS})
target_link_libraries(nexus-test PRIVATE nexus-api)

