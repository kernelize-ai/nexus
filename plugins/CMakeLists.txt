
if(LINUX)

    include(CheckLanguage)
    check_language(CUDA)

    if(CMAKE_CUDA_COMPILER)

        enable_language(CUDA)
        set(CUDA_AVAILABLE TRUE)
        add_subdirectory(cuda)

    else()

        set(CUDA_AVAILABLE FALSE)

    endif()

    add_subdirectory(hip)

elseif(MACOS)

    add_subdirectory(metal)

endif()
