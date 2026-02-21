#ifndef DRPC_COMMON_H
#define DRPC_COMMON_H

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <atomic>
#include <mutex>
#include <chrono>

namespace drpc {

using byte_t = uint8_t;
using byte_ptr = byte_t*;

struct Buffer {
    void* addr = nullptr;
    uint32_t lkey = 0;
    uint32_t rkey = 0;
    size_t size = 0;
    size_t slab_index = 0;

    Buffer() = default;
    Buffer(void* a, uint32_t lk, uint32_t rk, size_t s, size_t idx = 0)
        : addr(a), lkey(lk), rkey(rk), size(s), slab_index(idx) {}

    bool valid() const { return addr != nullptr && size > 0; }
    void reset() { addr = nullptr; lkey = 0; rkey = 0; size = 0; slab_index = 0; }
};

struct BufferSlice {
    void* addr = nullptr;
    uint32_t lkey = 0;
    size_t size = 0;

    BufferSlice() = default;
    BufferSlice(void* a, uint32_t lk, size_t s) : addr(a), lkey(lk), size(s) {}
    BufferSlice(const Buffer& buf, size_t offset, size_t len)
        : addr(static_cast<byte_ptr>(buf.addr) + offset),
          lkey(buf.lkey), size(len) {}

    bool valid() const { return addr != nullptr && size > 0; }
};

enum class ErrorCode : int {
    OK = 0,
    UNKNOWN = -1,
    INVALID_ARGUMENT = -2,
    OUT_OF_MEMORY = -3,
    DEVICE_NOT_FOUND = -4,
    RESOURCE_CREATION_FAILED = -5,
    CONNECTION_FAILED = -6,
    CONNECTION_CLOSED = -7,
    TIMEOUT = -8,
    SEND_FAILED = -9,
    RECV_FAILED = -10,
    REG_MR_FAILED = -11,
    POST_WR_FAILED = -12,
    POLL_CQ_FAILED = -13,
    QP_STATE_ERROR = -14,
};

class Status {
public:
    Status() : code_(ErrorCode::OK) {}
    explicit Status(ErrorCode code) : code_(code) {}
    Status(ErrorCode code, const std::string& msg) : code_(code), msg_(msg) {}

    bool ok() const { return code_ == ErrorCode::OK; }
    ErrorCode code() const { return code_; }
    const std::string& message() const { return msg_; }

    static Status OK() { return Status(); }
    static Status Error(ErrorCode code, const std::string& msg = "") {
        return Status(code, msg);
    }

private:
    ErrorCode code_;
    std::string msg_;
};

#define DRPC_MAGIC 0x5250
#define DRPC_VERSION 1

constexpr size_t DEFAULT_CQ_SIZE = 1024;
constexpr size_t DEFAULT_QP_DEPTH = 256;
constexpr size_t DEFAULT_MAX_INLINE = 128;
constexpr size_t DEFAULT_SEND_BUF_COUNT = 16;
constexpr size_t DEFAULT_RECV_BUF_COUNT = 16;

constexpr size_t SMALL_MSG_THRESHOLD = 128;

const std::vector<size_t> DEFAULT_SLAB_SIZES = {64, 256, 1024, 4096, 16384, 65536};
constexpr size_t DEFAULT_BLOCKS_PER_SLAB = 1024;

constexpr int DEFAULT_TIMEOUT_MS = 5000;
constexpr int DEFAULT_RETRY_COUNT = 7;
constexpr int DEFAULT_RNR_RETRY = 7;

}
#endif
