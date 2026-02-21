#include <drpc/connection.h>

namespace drpc {

Connection::Connection(std::unique_ptr<ConnectionManager> cm,
                       ProtectionDomain& pd,
                       CompletionQueue& send_cq,
                       CompletionQueue& recv_cq,
                       MemoryPool& pool) {
}

Connection::~Connection() {
}

bool Connection::valid() const {
    return false;
}

bool Connection::ready() const {
    return false;
}

void Connection::setPeerInfo(uint32_t rkey, uint64_t addr) {
}

Status Connection::close() {
    return Status::OK();
}

Status Connection::setupQP() {
    return Status::OK();
}

}
