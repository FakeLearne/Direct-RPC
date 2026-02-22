cmake_minimum_required(VERSION 3.14)
project(DirectRPC VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Threads REQUIRED)

find_library(IBVERBS_LIBRARY ibverbs)
find_path(IBVERBS_INCLUDE_DIR infiniband/verbs.h)

find_library(RDMACM_LIBRARY rdmacm)
find_path(RDMACM_INCLUDE_DIR rdma/rdma_cma.h)

if(NOT IBVERBS_LIBRARY OR NOT IBVERBS_INCLUDE_DIR)
    message(FATAL_ERROR "libibverbs not found")
endif()

if(NOT RDMACM_LIBRARY OR NOT RDMACM_INCLUDE_DIR)
    message(FATAL_ERROR "librdmacm not found")
endif()

add_library(drpc SHARED
    src/buffer/memory_pool.cpp
    src/transport/transport.cpp
    src/transport/connection.cpp
    src/protocol/codec.cpp
    src/rpc/rpc.cpp
)

target_include_directories(drpc
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${IBVERBS_INCLUDE_DIR}
        ${RDMACM_INCLUDE_DIR}
)

target_link_libraries(drpc
    PUBLIC Threads::Threads
    PRIVATE ${IBVERBS_LIBRARY} ${RDMACM_LIBRARY}
)

add_library(drpc_static STATIC
    src/buffer/memory_pool.cpp
    src/transport/transport.cpp
    src/transport/connection.cpp
    src/protocol/codec.cpp
    src/rpc/rpc.cpp
)

target_include_directories(drpc_static
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${IBVERBS_INCLUDE_DIR}
        ${RDMACM_INCLUDE_DIR}
)

target_link_libraries(drpc_static
    PUBLIC Threads::Threads
    PRIVATE ${IBVERBS_LIBRARY} ${RDMACM_LIBRARY}
)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
