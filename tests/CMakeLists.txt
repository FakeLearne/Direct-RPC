cmake_minimum_required(VERSION 3.14)

add_executable(unit_tests
    unit/test_main.cpp
    unit/test_memory_pool.cpp
    unit/test_codec.cpp
    unit/test_wr_builder.cpp
)

target_link_libraries(unit_tests PRIVATE drpc_static)

add_test(NAME unit_tests COMMAND unit_tests)

add_executable(connection_test
    integration/connection_test.cpp
)

target_link_libraries(connection_test PRIVATE drpc_static)

add_executable(transport_test
    integration/transport_test.cpp
)

target_link_libraries(transport_test PRIVATE drpc_static)

add_executable(rpc_test
    integration/rpc_test.cpp
)

target_link_libraries(rpc_test PRIVATE drpc_static)
