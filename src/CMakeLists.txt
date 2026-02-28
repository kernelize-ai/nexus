project(nexus-api)

set(CMAKE_CXX_VISIBILITY_PRESET default)

add_library(nexus-api SHARED
    buffer.cpp
    info.cpp
    kernel.cpp
    library.cpp
    command.cpp
    event.cpp
    schedule.cpp
    device.cpp
    device_db.cpp
    utility.cpp
    stream.cpp
    system.cpp
    runtime.cpp
)

target_link_libraries(nexus-api PRIVATE nxs-api)
target_link_libraries(nexus-api PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(nexus-api PRIVATE ${CMAKE_DL_LIBS})
