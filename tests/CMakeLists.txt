cmake_minimum_required(VERSION 3.14)

add_executable(unit_test unit_test.cpp)
target_link_libraries(unit_test PRIVATE drpc_static)

add_executable(integration_test integration_test.cpp)
target_link_libraries(integration_test PRIVATE drpc_static)

add_executable(rpc_test rpc_test.cpp)
target_link_libraries(rpc_test PRIVATE drpc_static)
