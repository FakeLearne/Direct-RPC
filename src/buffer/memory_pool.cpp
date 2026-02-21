#include <drpc/buffer.h>
#include <drpc/transport.h>

namespace drpc {

MemoryPool::MemoryPool(ProtectionDomain& pd, const std::vector<size_t>& block_sizes, size_t blocks_per_slab) {
}

MemoryPool::~MemoryPool() {
}

Buffer MemoryPool::allocate(size_t size) {
    return Buffer();
}

void MemoryPool::deallocate(Buffer& buf) {
}

size_t MemoryPool::totalAllocated() const {
    return 0;
}

size_t MemoryPool::totalCapacity() const {
    return 0;
}

}
