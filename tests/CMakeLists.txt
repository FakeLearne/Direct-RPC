cmake_minimum_required(VERSION 3.14)

add_executable(rpc_test rpc_test.cpp)
target_link_libraries(rpc_test PRIVATE drpc_static)
