#ifndef DRPC_BUFFER_H
#define DRPC_BUFFER_H

#include <drpc/common.h>
#include <infiniband/verbs.h>

namespace drpc {

// 内存池，管理预注册的RDMA内存
class MemoryPool {
public:
    // 构造函数，创建多个Slab用于不同大小的内存分配
    MemoryPool(ibv_pd* pd, 
               const std::vector<size_t>& block_sizes = DEFAULT_SLAB_SIZES,
               size_t blocks_per_slab = DEFAULT_BLOCKS_PER_SLAB);
    ~MemoryPool();

    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    Buffer allocate(size_t size);     // 分配指定大小的缓冲区
    void deallocate(Buffer& buf);     // 释放缓冲区

private:
    // Slab结构，管理一块连续的预注册内存
    struct Slab {
        void* base_addr;              // 内存基地址
        ibv_mr* mr;                   // 内存区域句柄
        std::vector<bool> free_map;   // 空闲块位图
        size_t block_size;            // 每块大小
        size_t block_count;           // 总块数
        size_t free_count;            // 空闲块数
    };

    ibv_pd* pd_;                      // 保护域
    std::vector<Slab> slabs_;         // Slab列表
    std::mutex mutex_;                // 线程安全锁
};

}
#endif
