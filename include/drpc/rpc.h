#ifndef DRPC_RPC_H
#define DRPC_RPC_H

#include <drpc/common.h>
#include <drpc/transport.h>
#include <drpc/protocol.h>
#include <memory>

namespace drpc {

// RPC处理函数类型
using RpcHandler = std::function<void(const void* req, size_t req_len,
                                      void* resp, size_t& resp_len)>;

// RPC服务端
class RpcServer {
public:
    RpcServer(const char* addr, uint16_t port);
    ~RpcServer();

    RpcServer(const RpcServer&) = delete;
    RpcServer& operator=(const RpcServer&) = delete;

    void registerHandler(uint32_t func_id, RpcHandler handler);  // 注册RPC处理函数
    
    ErrorCode start();                   // 启动服务，返回错误码
    void stop();                         // 停止服务
    void run();                          // 运行事件循环

    bool running() const { return running_.load(); }

private:
    void handleClient(Connection& conn);
    void processRequest(Connection& conn, const MsgHeader* hdr);

    std::string addr_;
    uint16_t port_;
    Transport transport_;
    rdma_cm_id* listen_id_ = nullptr;
    std::unordered_map<uint32_t, RpcHandler> handlers_;
    std::atomic<bool> running_{false};
};

// RPC客户端
class RpcClient {
public:
    RpcClient();
    ~RpcClient();

    RpcClient(const RpcClient&) = delete;
    RpcClient& operator=(const RpcClient&) = delete;

    ErrorCode connect(const char* addr, uint16_t port, int timeout_ms = DEFAULT_TIMEOUT_MS);
    void disconnect();
    
    // 同步RPC调用（Push模式，使用Send/Recv）
    ErrorCode call(uint32_t func_id, const void* req, size_t req_len,
                   void* resp, size_t& resp_len, int timeout_ms = DEFAULT_TIMEOUT_MS);

    // Pull模式RPC调用（使用RDMA Read）
    ErrorCode callWithPull(uint32_t func_id, const void* req, size_t req_len,
                           void* resp, size_t& resp_len, int timeout_ms = DEFAULT_TIMEOUT_MS);

    bool connected() const { return conn_ && conn_->connected(); }

private:
    ErrorCode waitForResponse(uint32_t seq_id, Buffer& recv_buf, int timeout_ms);
    ErrorCode waitForSendComplete(int timeout_ms);

    Transport transport_;
    std::unique_ptr<Connection> conn_;
    std::atomic<uint32_t> next_seq_id_{1};
};

}
#endif
