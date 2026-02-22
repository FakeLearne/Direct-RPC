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
#include <condition_variable>

namespace drpc {

// RDMA缓冲区，包含本地地址、lkey、rkey和大小
struct Buffer {
    void* addr = nullptr;   // 缓冲区地址
    uint32_t lkey = 0;      // 本地访问key
    uint32_t rkey = 0;      // 远程访问key
    size_t size = 0;        // 缓冲区大小

    Buffer() = default;
    Buffer(void* a, uint32_t lk, uint32_t rk, size_t s)
        : addr(a), lkey(lk), rkey(rk), size(s) {}

    bool valid() const { return addr != nullptr && size > 0; }  // 检查缓冲区是否有效
    void reset() { addr = nullptr; lkey = 0; rkey = 0; size = 0; }  // 重置缓冲区
};

constexpr size_t DEFAULT_CQ_SIZE = 1024;    // CQ 默认容量
constexpr size_t DEFAULT_QP_DEPTH = 256;    // QP 默认深度
constexpr size_t DEFAULT_MAX_INLINE = 128;  // 最大 inline 数据大小
constexpr size_t SMALL_MSG_THRESHOLD = 128; // 小消息阈值

// 内存池相关常量
const std::vector<size_t> DEFAULT_SLAB_SIZES = {64, 256, 1024, 4096, 16384, 65536}; // Slab 大小配置
constexpr size_t DEFAULT_BLOCKS_PER_SLAB = 1024;    // 每个 Slab 的块数

// 超时相关常量
constexpr int DEFAULT_TIMEOUT_MS = 5000;    // 默认超时时间（ms）
constexpr int DEFAULT_RETRY_COUNT = 7;      // 默认重试次数

}
#endif
