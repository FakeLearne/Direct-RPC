#include <drpc/buffer.h>
#include <cstring>

namespace drpc {

// 构造函数：为每种块大小创建一个Slab，注册内存
MemoryPool::MemoryPool(ibv_pd* pd, const std::vector<size_t>& block_sizes,
                       size_t blocks_per_slab) : pd_(pd) {
    for (size_t bs : block_sizes) {
        Slab slab;
        slab.block_size = bs;
        slab.block_count = blocks_per_slab;
        slab.free_count = blocks_per_slab;
        slab.free_map.resize(blocks_per_slab, true);
        
        // 分配内存
        size_t total_size = bs * blocks_per_slab;
        slab.base_addr = ::malloc(total_size);
        if (!slab.base_addr) continue;
        
        // 注册MR，允许本地和远程读写
        slab.mr = ibv_reg_mr(pd, slab.base_addr, total_size,
                             IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ);
        if (!slab.mr) {
            ::free(slab.base_addr);
            continue;
        }
        
        slabs_.push_back(std::move(slab));
    }
}

// 析构函数：注销MR并释放内存
MemoryPool::~MemoryPool() {
    for (auto& slab : slabs_) {
        if (slab.mr) {
            ibv_dereg_mr(slab.mr);
        }
        if (slab.base_addr) {
            ::free(slab.base_addr);
        }
    }
}

// 分配缓冲区：找到合适大小的Slab，分配一个空闲块
Buffer MemoryPool::allocate(size_t size) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& slab : slabs_) {
        if (slab.block_size >= size && slab.free_count > 0) {
            for (size_t i = 0; i < slab.block_count; ++i) {
                if (slab.free_map[i]) {
                    slab.free_map[i] = false;
                    slab.free_count--;
                    
                    void* addr = static_cast<char*>(slab.base_addr) + i * slab.block_size;
                    return Buffer(addr, slab.mr->lkey, slab.mr->rkey, slab.block_size);
                }
            }
        }
    }
    return Buffer();  // 分配失败，返回空Buffer
}

// 释放缓冲区：找到对应的Slab，标记块为空闲
void MemoryPool::deallocate(Buffer& buf) {
    if (!buf.valid()) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& slab : slabs_) {
        char* base = static_cast<char*>(slab.base_addr);
        char* ptr = static_cast<char*>(buf.addr);
        
        // 检查是否属于这个Slab
        if (ptr >= base && ptr < base + slab.block_size * slab.block_count) {
            size_t index = (ptr - base) / slab.block_size;
            if (!slab.free_map[index]) {
                slab.free_map[index] = true;
                slab.free_count++;
            }
            buf.reset();
            return;
        }
    }
}

}
