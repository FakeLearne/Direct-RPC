cmake_minimum_required(VERSION 3.14)

add_executable(simple_server simple_server.cpp)
target_link_libraries(simple_server PRIVATE drpc_static)

add_executable(simple_client simple_client.cpp)
target_link_libraries(simple_client PRIVATE drpc_static)

add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark PRIVATE drpc_static)
