project(nexus-api)

set(CMAKE_CXX_VISIBILITY_PRESET default)

add_library(nexus-api SHARED
    buffer.cpp
    properties.cpp
    kernel.cpp
    library.cpp
    command.cpp
    schedule.cpp
    device.cpp
    device_db.cpp
    utility.cpp
    stream.cpp
    system.cpp
    runtime.cpp
    ${CMAKE_SOURCE_DIR}/api-src/nxs_api.cpp
)

target_link_libraries(nexus-api PRIVATE nlohmann_json::nlohmann_json)
