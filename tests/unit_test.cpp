#include <iostream>
#include <cstring>
#include <cassert>
#include <drpc/common.h>
#include <drpc/protocol.h>
#include <drpc/buffer.h>

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
#define ASSERT_FALSE(cond) if (cond) throw std::runtime_error("Assertion failed: " #cond)
#define ASSERT_EQ(a, b) if ((a) != (b)) throw std::runtime_error("Assertion failed: " #a " != " #b)
#define ASSERT_NE(a, b) if ((a) == (b)) throw std::runtime_error("Assertion failed: " #a " == " #b)

TEST(error_code_is_ok) {
    ASSERT_TRUE(is_ok(ErrorCode::OK));
    ASSERT_FALSE(is_ok(ErrorCode::ERROR));
    ASSERT_FALSE(is_ok(ErrorCode::TIMEOUT));
    ASSERT_FALSE(is_ok(ErrorCode::CONNECT_FAILED));
}

TEST(error_code_to_string) {
    ASSERT_EQ(std::string(error_str(ErrorCode::OK)), "OK");
    ASSERT_EQ(std::string(error_str(ErrorCode::ERROR)), "ERROR");
    ASSERT_EQ(std::string(error_str(ErrorCode::TIMEOUT)), "TIMEOUT");
    ASSERT_EQ(std::string(error_str(ErrorCode::CONNECT_FAILED)), "CONNECT_FAILED");
    ASSERT_EQ(std::string(error_str(static_cast<ErrorCode>(-999))), "UNKNOWN");
}

TEST(buffer_default_construction) {
    Buffer buf;
    ASSERT_EQ(buf.addr, nullptr);
    ASSERT_EQ(buf.lkey, 0u);
    ASSERT_EQ(buf.rkey, 0u);
    ASSERT_EQ(buf.size, 0u);
    ASSERT_FALSE(buf.valid());
}

TEST(buffer_parameterized_construction) {
    char data[128];
    Buffer buf(data, 100, 200, 128);
    ASSERT_EQ(buf.addr, data);
    ASSERT_EQ(buf.lkey, 100u);
    ASSERT_EQ(buf.rkey, 200u);
    ASSERT_EQ(buf.size, 128u);
    ASSERT_TRUE(buf.valid());
}

TEST(buffer_reset) {
    char data[64];
    Buffer buf(data, 1, 2, 64);
    ASSERT_TRUE(buf.valid());
    buf.reset();
    ASSERT_FALSE(buf.valid());
    ASSERT_EQ(buf.addr, nullptr);
}

TEST(message_header_size) {
    ASSERT_EQ(MSG_HEADER_SIZE, 20u);
}

TEST(message_header_magic) {
    ASSERT_EQ(MSG_MAGIC, 0x5250u);
}

TEST(message_type_values) {
    ASSERT_EQ(static_cast<int>(MsgType::MSG_REQUEST), 0);
    ASSERT_EQ(static_cast<int>(MsgType::MSG_RESPONSE), 1);
    ASSERT_EQ(static_cast<int>(MsgType::MSG_ACK), 2);
    ASSERT_EQ(static_cast<int>(MsgType::MSG_ERROR), 3);
    ASSERT_EQ(static_cast<int>(MsgType::MSG_DATA_READY), 4);
}

TEST(message_flags) {
    ASSERT_EQ(static_cast<int>(FLAG_NONE), 0);
    ASSERT_EQ(static_cast<int>(FLAG_INLINE), 1);
    ASSERT_EQ(static_cast<int>(FLAG_HAS_PAYLOAD), 2);
    ASSERT_EQ(static_cast<int>(FLAG_PULL_MODE), 4);
}

TEST(message_header_layout) {
    MsgHeader hdr;
    hdr.magic = MSG_MAGIC;
    hdr.version = 1;
    hdr.type = static_cast<uint8_t>(MsgType::MSG_REQUEST);
    hdr.flags = FLAG_INLINE;
    hdr.seq_id = 12345;
    hdr.func_id = 100;
    hdr.payload_len = 256;
    
    ASSERT_EQ(hdr.magic, MSG_MAGIC);
    ASSERT_EQ(hdr.version, 1u);
    ASSERT_EQ(hdr.type, static_cast<uint8_t>(MsgType::MSG_REQUEST));
    ASSERT_EQ(hdr.flags, FLAG_INLINE);
    ASSERT_EQ(hdr.seq_id, 12345u);
    ASSERT_EQ(hdr.func_id, 100u);
    ASSERT_EQ(hdr.payload_len, 256u);
}

TEST(data_ready_msg_size) {
    ASSERT_EQ(DATA_READY_MSG_SIZE, sizeof(DataReadyMsg));
}

TEST(data_ready_msg_layout) {
    DataReadyMsg msg;
    msg.remote_addr = 0x12345678ABCDEF00ULL;
    msg.rkey = 12345;
    msg.data_len = 1024;
    msg.seq_id = 999;
    
    ASSERT_EQ(msg.remote_addr, 0x12345678ABCDEF00ULL);
    ASSERT_EQ(msg.rkey, 12345u);
    ASSERT_EQ(msg.data_len, 1024u);
    ASSERT_EQ(msg.seq_id, 999u);
}

TEST(validate_header_valid) {
    MsgHeader hdr;
    hdr.magic = MSG_MAGIC;
    hdr.version = 1;
    hdr.type = static_cast<uint8_t>(MsgType::MSG_REQUEST);
    hdr.flags = 0;
    hdr.seq_id = 1;
    hdr.func_id = 1;
    hdr.payload_len = 0;
    
    ASSERT_TRUE(validateHeader(&hdr));
}

TEST(validate_header_invalid_magic) {
    MsgHeader hdr;
    hdr.magic = 0x1234;
    hdr.version = 1;
    hdr.type = static_cast<uint8_t>(MsgType::MSG_REQUEST);
    hdr.flags = 0;
    hdr.seq_id = 1;
    hdr.func_id = 1;
    hdr.payload_len = 0;
    
    ASSERT_FALSE(validateHeader(&hdr));
}

TEST(validate_header_null) {
    ASSERT_FALSE(validateHeader(nullptr));
}

TEST(msg_total_size) {
    MsgHeader hdr;
    hdr.magic = MSG_MAGIC;
    hdr.payload_len = 100;
    
    ASSERT_EQ(msgTotalSize(&hdr), MSG_HEADER_SIZE + 100);
}

TEST(get_msg_header) {
    char buffer[256];
    MsgHeader* hdr = getMsgHeader(buffer);
    hdr->magic = MSG_MAGIC;
    hdr->seq_id = 123;
    
    const MsgHeader* chdr = getMsgHeader(static_cast<const void*>(buffer));
    ASSERT_EQ(chdr->magic, MSG_MAGIC);
    ASSERT_EQ(chdr->seq_id, 123u);
}

TEST(get_msg_payload) {
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    
    void* payload = getMsgPayload(buffer);
    ASSERT_EQ(static_cast<char*>(payload) - buffer, static_cast<ptrdiff_t>(MSG_HEADER_SIZE));
    
    const void* cpayload = getMsgPayload(static_cast<const void*>(buffer));
    ASSERT_EQ(static_cast<const char*>(cpayload) - buffer, static_cast<ptrdiff_t>(MSG_HEADER_SIZE));
}

int main() {
    std::cout << "=== Unit Tests ===" << std::endl;
    
    RUN_TEST(error_code_is_ok);
    RUN_TEST(error_code_to_string);
    RUN_TEST(buffer_default_construction);
    RUN_TEST(buffer_parameterized_construction);
    RUN_TEST(buffer_reset);
    RUN_TEST(message_header_size);
    RUN_TEST(message_header_magic);
    RUN_TEST(message_type_values);
    RUN_TEST(message_flags);
    RUN_TEST(message_header_layout);
    RUN_TEST(data_ready_msg_size);
    RUN_TEST(data_ready_msg_layout);
    RUN_TEST(validate_header_valid);
    RUN_TEST(validate_header_invalid_magic);
    RUN_TEST(validate_header_null);
    RUN_TEST(msg_total_size);
    RUN_TEST(get_msg_header);
    RUN_TEST(get_msg_payload);
    
    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << tests_failed << std::endl;
    
    return tests_failed > 0 ? 1 : 0;
}
