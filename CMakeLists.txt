cmake_minimum_required(VERSION 3.16)

project(CP_OS_2025)

set(CMAKE_CXX_STANDARD 17)

add_executable(server
    server/server.cpp
)

target_include_directories(server PRIVATE
    server
)

target_link_libraries(server PRIVATE
    zmq
)

# ---- Client ----
add_executable(client
    client/client.cpp
)

target_include_directories(client PRIVATE
    client
)

target_link_libraries(client PRIVATE
    zmq
    pthread
)
