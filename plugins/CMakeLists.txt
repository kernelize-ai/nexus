
if(LINUX)

    add_subdirectory(cuda)
    add_subdirectory(hip)

elseif(MACOS)

    add_subdirectory(metal)

endif()
