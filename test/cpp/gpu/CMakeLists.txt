project(nexus-test)

if(MACOS)

    #add_executable(nexus-test MACOSX_BUNDLE main.cpp
    add_executable(nexus-test main.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/kernel.air ${CMAKE_CURRENT_BINARY_DIR}/kernel.metallib)
    #set_target_properties(nexus-test PROPERTIES OUTPUT_NAME bare_metal_tri)
    target_link_libraries(nexus-test PRIVATE nexus-api)

    add_custom_command(OUTPUT kernel.air
      MAIN_DEPENDENCY kernel.metal
      COMMAND xcrun -sdk macosx metal -c ${CMAKE_CURRENT_SOURCE_DIR}/kernel.metal -o ${CMAKE_CURRENT_BINARY_DIR}/kernel.air)

    add_custom_command(OUTPUT kernel.metallib
      MAIN_DEPENDENCY kernel.air
      COMMAND xcrun -sdk macosx metallib ${CMAKE_CURRENT_BINARY_DIR}/kernel.air -o ${CMAKE_CURRENT_BINARY_DIR}/kernel.metallib)

endif()

if(LINUX)

    add_executable(nexus-test-linux 
    test_linux.cpp )
    target_link_libraries(nexus-test-linux PRIVATE nexus-api)

endif()
