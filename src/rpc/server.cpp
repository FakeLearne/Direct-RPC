#include <drpc/rpc.h>

namespace drpc {

RpcServer::RpcServer(const char* addr, uint16_t port)
    : bind_addr_(addr), bind_port_(port) {
}

RpcServer::~RpcServer() {
    stop();
}

void RpcServer::registerHandler(uint32_t func_id, RPCHandler handler) {
}

Status RpcServer::start() {
    return Status::OK();
}

void RpcServer::stop() {
}

size_t RpcServer::connectionCount() const {
    return 0;
}

void RpcServer::acceptLoop() {
}

void RpcServer::handleConnection(std::unique_ptr<Connection> conn) {
}

void RpcServer::handleRequest(Connection& conn, const MessageHeader& hdr, const void* data) {
}

}
