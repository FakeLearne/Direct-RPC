#include <drpc/dispatcher.h>

namespace drpc {

uint64_t RequestContextManager::createContext(uint32_t seq_id, uint32_t func_id,
                                               Buffer& send_buf, Buffer& recv_buf,
                                               std::function<void(Buffer&)> callback) {
    return 0;
}

RequestContext* RequestContextManager::getContext(uint64_t id) {
    return nullptr;
}

void RequestContextManager::removeContext(uint64_t id) {
}

bool RequestContextManager::hasContext(uint64_t id) const {
    return false;
}

size_t RequestContextManager::pendingCount() const {
    return 0;
}

void RequestContextManager::setTimeout(uint64_t id, int timeout_ms) {
}

std::vector<uint64_t> RequestContextManager::getTimedOutRequests() {
    return {};
}

}
