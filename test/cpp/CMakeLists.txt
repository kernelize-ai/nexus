project(nexus-test)

if(MACOS)

    add_executable(nexus-test 
    main.cpp )
    target_link_libraries(nexus-test PRIVATE nexus-api)

endif()

if(LINUX)

    add_executable(nexus-test-linux 
    test_linux.cpp )
    target_link_libraries(nexus-test-linux PRIVATE nexus-api)

endif()
