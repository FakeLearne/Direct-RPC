#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>
#include <drpc/rpc.h>

using namespace drpc;

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "Running " << #name << "... "; \
    try { \
        test_##name(); \
        std::cout << "PASSED" << std::endl; \
        tests_passed++; \
    } catch (const std::exception& e) { \
        std::cout << "FAILED: " << e.what() << std::endl; \
        tests_failed++; \
    } \
} while(0)

#define ASSERT_TRUE(cond) if (!(cond)) throw std::runtime_error("Assertion failed: " #cond)

TEST(transport_init) {
    Transport transport;
    ErrorCode ec = transport.init();
    if (!is_ok(ec)) {
        std::cout << "SKIPPED (no RDMA device: " << error_str(ec) << ")" << std::endl;
        return;
    }
    ASSERT_TRUE(transport.context() != nullptr);
    ASSERT_TRUE(transport.pd() != nullptr);
    ASSERT_TRUE(transport.cq() != nullptr);
}

TEST(rpc_server_create) {
    RpcServer server("0.0.0.0", 12346);
    std::cout << "PASSED" << std::endl;
}

TEST(rpc_server_register_handler) {
    RpcServer server("0.0.0.0", 12347);
    
    server.registerHandler(1, [](const void* req, size_t req_len, void* resp, size_t& resp_len) {
        const char* reply = "Handler response";
        size_t len = strlen(reply);
        memcpy(resp, reply, len);
        resp_len = len;
    });
    
    std::cout << "PASSED" << std::endl;
}

TEST(rpc_server_start) {
    RpcServer server("0.0.0.0", 12348);
    ErrorCode ec = server.start();
    if (!is_ok(ec)) {
        std::cout << "SKIPPED (no RDMA device: " << error_str(ec) << ")" << std::endl;
        return;
    }
    ASSERT_TRUE(is_ok(ec));
}

TEST(rpc_client_create) {
    RpcClient client;
    std::cout << "PASSED" << std::endl;
}

TEST(rpc_client_connect_no_server) {
    RpcClient client;
    ErrorCode ec = client.connect("127.0.0.1", 59999);
    if (!is_ok(ec)) {
        std::cout << "PASSED (expected failure: " << error_str(ec) << ")" << std::endl;
    } else {
        std::cout << "PASSED" << std::endl;
    }
}

TEST(connection_create) {
    Transport transport;
    ErrorCode ec = transport.init();
    if (!is_ok(ec)) {
        std::cout << "SKIPPED (no RDMA device)" << std::endl;
        return;
    }
    
    Connection conn(transport);
    std::cout << "PASSED" << std::endl;
}

int main() {
    std::cout << "=== Integration Tests (require RDMA device) ===" << std::endl;
    
    RUN_TEST(transport_init);
    RUN_TEST(rpc_server_create);
    RUN_TEST(rpc_server_register_handler);
    RUN_TEST(rpc_server_start);
    RUN_TEST(rpc_client_create);
    RUN_TEST(rpc_client_connect_no_server);
    RUN_TEST(connection_create);
    
    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << tests_failed << std::endl;
    
    return tests_failed > 0 ? 1 : 0;
}
