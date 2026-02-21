#include <drpc/buffer.h>

namespace drpc {

MemoryRegion::MemoryRegion(ProtectionDomain& pd, void* addr, size_t size, int access) {
}

MemoryRegion::~MemoryRegion() {
}

MemoryRegion::MemoryRegion(MemoryRegion&& other) noexcept {
}

MemoryRegion& MemoryRegion::operator=(MemoryRegion&& other) noexcept {
    return *this;
}

}
