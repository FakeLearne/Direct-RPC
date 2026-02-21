#ifndef DRPC_BUFFER_H
#define DRPC_BUFFER_H

#include <drpc/common.h>
#include <infiniband/verbs.h>

namespace drpc {

class ProtectionDomain;

class MemoryRegion {
public:
    MemoryRegion(ProtectionDomain& pd, void* addr, size_t size,
                 int access = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ);
    ~MemoryRegion();

    MemoryRegion(const MemoryRegion&) = delete;
    MemoryRegion& operator=(const MemoryRegion&) = delete;
    MemoryRegion(MemoryRegion&& other) noexcept;
    MemoryRegion& operator=(MemoryRegion&& other) noexcept;

    bool valid() const { return mr_ != nullptr; }
    uint32_t lkey() const { return mr_ ? mr_->lkey : 0; }
    uint32_t rkey() const { return mr_ ? mr_->rkey : 0; }
    void* addr() const { return addr_; }
    size_t size() const { return size_; }
    ibv_mr* get() const { return mr_; }

private:
    ibv_mr* mr_ = nullptr;
    void* addr_ = nullptr;
    size_t size_ = 0;
};

class MemoryPool {
public:
    struct SlabConfig {
        size_t block_size;
        size_t block_count;
    };

    MemoryPool(ProtectionDomain& pd,
               const std::vector<size_t>& block_sizes = DEFAULT_SLAB_SIZES,
               size_t blocks_per_slab = DEFAULT_BLOCKS_PER_SLAB);
    ~MemoryPool();

    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    Buffer allocate(size_t size);
    void deallocate(Buffer& buf);

    size_t totalAllocated() const;
    size_t totalCapacity() const;
    size_t slabCount() const { return slabs_.size(); }

private:
    struct Slab {
        void* base_addr;
        std::unique_ptr<MemoryRegion> mr;
        std::vector<bool> free_map;
        size_t block_size;
        size_t block_count;
        size_t free_count;
    };

    std::vector<Slab> slabs_;
    mutable std::mutex mutex_;
    size_t total_allocated_ = 0;
};

class ConnectionBuffer {
public:
    ConnectionBuffer(MemoryPool& pool, size_t buf_count = DEFAULT_SEND_BUF_COUNT);
    ~ConnectionBuffer();

    ConnectionBuffer(const ConnectionBuffer&) = delete;
    ConnectionBuffer& operator=(const ConnectionBuffer&) = delete;

    Buffer& getSendBuffer();
    Buffer& getRecvBuffer();
    void returnSendBuffer(Buffer& buf);
    void returnRecvBuffer(Buffer& buf);

    size_t availableSendBuffers() const;
    size_t availableRecvBuffers() const;

    void refillRecvBuffers();

private:
    MemoryPool& pool_;
    std::vector<Buffer> send_bufs_;
    std::vector<Buffer> recv_bufs_;
    std::vector<bool> send_in_use_;
    std::vector<bool> recv_in_use_;
    size_t send_index_ = 0;
    size_t recv_index_ = 0;
};

}
#endif
