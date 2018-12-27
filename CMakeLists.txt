cmake_minimum_required(VERSION 2.8.9)
project(Message_Subscription)

set(CMAKE_CXX_STANDARD 11)

include_directories(
        .
        dep/bounce
        dep/json/include
        dep/bounce/deps/
)

add_subdirectory(dep/bounce)

add_executable(server
        server.cc
        server_main.cc)
target_link_libraries(server bounce)

add_executable(client
        client.cc
        client_main.cc)
target_link_libraries(client bounce)