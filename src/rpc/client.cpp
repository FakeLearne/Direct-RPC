#include <drpc/rpc.h>

namespace drpc {

RpcClient::RpcClient() {
}

RpcClient::~RpcClient() {
    disconnect();
}

Status RpcClient::connect(const char* addr, uint16_t port, int timeout_ms) {
    return Status::OK();
}

void RpcClient::disconnect() {
}

Buffer RpcClient::call(uint32_t func_id, const void* req, size_t req_len, int timeout_ms) {
    return doSyncCall(func_id, req, req_len, timeout_ms, CallMode::SYNC);
}

Buffer RpcClient::callWithPull(uint32_t func_id, const void* req, size_t req_len, int timeout_ms) {
    return doSyncCall(func_id, req, req_len, timeout_ms, CallMode::PULL);
}

uint32_t RpcClient::asyncCall(uint32_t func_id, const void* req, size_t req_len,
                               std::function<void(Buffer&)> callback) {
    return 0;
}

bool RpcClient::wait(uint32_t seq_id, int timeout_ms) {
    return false;
}

bool RpcClient::waitAll(int timeout_ms) {
    return false;
}

Buffer RpcClient::doSyncCall(uint32_t func_id, const void* req, size_t req_len,
                              int timeout_ms, CallMode mode) {
    return Buffer();
}

void RpcClient::handleDataReady(const DataReadyInfo& info, Buffer& recv_buf, uint32_t seq_id) {
}

}
