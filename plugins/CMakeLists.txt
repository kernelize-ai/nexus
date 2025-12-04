
include_directories(include)

add_subdirectory(cpu)

if(LINUX)

    include(CheckLanguage)
    check_language(CUDA)

    if(CMAKE_CUDA_COMPILER)

        message(STATUS "CUDA compiler found")

        enable_language(CUDA)
        set(CUDA_AVAILABLE TRUE)

        find_package(CUDAToolkit REQUIRED)

        add_subdirectory(cuda)


    else()

        set(CUDA_AVAILABLE FALSE)

    endif()

    add_subdirectory(hip)

elseif(MACOS)

    add_subdirectory(metal)

endif()

add_subdirectory(tenstorrent)