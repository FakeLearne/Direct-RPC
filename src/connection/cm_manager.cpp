#include <drpc/connection.h>

namespace drpc {

ConnectionManager::ConnectionManager(bool is_server) {
}

ConnectionManager::~ConnectionManager() {
}

Status ConnectionManager::bind(const char* addr, uint16_t port) {
    return Status::OK();
}

Status ConnectionManager::listen(int backlog) {
    return Status::OK();
}

std::unique_ptr<ConnectionManager> ConnectionManager::accept() {
    return nullptr;
}

Status ConnectionManager::connect(const char* addr, uint16_t port, int timeout_ms) {
    return Status::OK();
}

Status ConnectionManager::disconnect() {
    return Status::OK();
}

int ConnectionManager::getEvent(rdma_cm_event** event) {
    return 0;
}

void ConnectionManager::ackEvent(rdma_cm_event* event) {
}

uint16_t ConnectionManager::localPort() const {
    return 0;
}

const char* ConnectionManager::localAddr() const {
    return "";
}

}
