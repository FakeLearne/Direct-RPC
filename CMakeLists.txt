cmake_minimum_required(VERSION 3.14)
project(DirectRPC VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTS "Build unit and integration tests" ON)
option(BUILD_EXAMPLES "Build example programs" ON)
option(ENABLE_PROFILING "Enable internal profiling" ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Threads REQUIRED)

find_library(IBVERBS_LIBRARY ibverbs)
find_path(IBVERBS_INCLUDE_DIR infiniband/verbs.h)

find_library(RDMACM_LIBRARY rdmacm)
find_path(RDMACM_INCLUDE_DIR rdma/rdma_cma.h)

if(NOT IBVERBS_LIBRARY OR NOT IBVERBS_INCLUDE_DIR)
    message(FATAL_ERROR "libibverbs not found. Please install ibverbs-devel or infiniband-devel package.")
endif()

if(NOT RDMACM_LIBRARY OR NOT RDMACM_INCLUDE_DIR)
    message(FATAL_ERROR "librdmacm not found. Please install rdmacm-devel or rdma-core-devel package.")
endif()

message(STATUS "Found ibverbs: ${IBVERBS_LIBRARY}")
message(STATUS "Found rdmacm: ${RDMACM_LIBRARY}")

add_library(drpc SHARED
    src/buffer/memory_pool.cpp
    src/buffer/memory_region.cpp
    src/transport/context.cpp
    src/transport/pd.cpp
    src/transport/cq.cpp
    src/transport/qp.cpp
    src/transport/wr_builder.cpp
    src/transport/transport.cpp
    src/connection/cm_manager.cpp
    src/connection/connection.cpp
    src/protocol/message.cpp
    src/protocol/codec.cpp
    src/dispatcher/dispatcher.cpp
    src/dispatcher/request_context.cpp
    src/rpc/client.cpp
    src/rpc/server.cpp
    src/profiling/profiler.cpp
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
    PUBLIC
        Threads::Threads
    PRIVATE
        ${IBVERBS_LIBRARY}
        ${RDMACM_LIBRARY}
)

target_compile_options(drpc PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

if(ENABLE_PROFILING)
    target_compile_definitions(drpc PRIVATE DRPC_ENABLE_PROFILING=1)
endif()

add_library(drpc_static STATIC
    src/buffer/memory_pool.cpp
    src/buffer/memory_region.cpp
    src/transport/context.cpp
    src/transport/pd.cpp
    src/transport/cq.cpp
    src/transport/qp.cpp
    src/transport/wr_builder.cpp
    src/transport/transport.cpp
    src/connection/cm_manager.cpp
    src/connection/connection.cpp
    src/protocol/message.cpp
    src/protocol/codec.cpp
    src/dispatcher/dispatcher.cpp
    src/dispatcher/request_context.cpp
    src/rpc/client.cpp
    src/rpc/server.cpp
    src/profiling/profiler.cpp
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
    PUBLIC
        Threads::Threads
    PRIVATE
        ${IBVERBS_LIBRARY}
        ${RDMACM_LIBRARY}
)

target_compile_options(drpc_static PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

if(ENABLE_PROFILING)
    target_compile_definitions(drpc_static PRIVATE DRPC_ENABLE_PROFILING=1)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

install(TARGETS drpc drpc_static
    EXPORT drpc-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/drpc
    DESTINATION include
)

install(EXPORT drpc-targets
    FILE drpc-config.cmake
    NAMESPACE drpc::
    DESTINATION lib/cmake/drpc
)
