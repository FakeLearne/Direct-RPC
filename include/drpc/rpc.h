#ifndef DRPC_RPC_H
#define DRPC_RPC_H

#include <drpc/common.h>
#include <drpc/transport.h>
#include <drpc/protocol.h>

namespace drpc {

// RPC处理函数类型
using RpcHandler = std::function<void(const void* req, size_t req_len,
                                      void* resp, size_t& resp_len)>;

// RPC服务端
class RpcServer {
public:
    RpcServer(const char* addr, uint16_t port);  // 构造，指定监听地址和端口
    ~RpcServer();

    RpcServer(const RpcServer&) = delete;
    RpcServer& operator=(const RpcServer&) = delete;

    void registerHandler(uint32_t func_id, RpcHandler handler);  // 注册RPC处理函数
    
    bool start();                        // 启动服务
    void stop();                         // 停止服务
    void run();                          // 运行事件循环

private:
    void handleClient(Connection& conn);            // 处理客户端连接
    void processRequest(Connection& conn, const MsgHeader* hdr);  // 处理RPC请求

    std::string addr_;                   // 监听地址
    uint16_t port_;                      // 监听端口
    Transport transport_;                // RDMA传输层
    rdma_cm_id* listen_id_ = nullptr;    // rdma_cm监听ID
    std::unordered_map<uint32_t, RpcHandler> handlers_;  // RPC处理函数表
    std::atomic<bool> running_{false};   // 运行状态
};

// RPC客户端
class RpcClient {
public:
    RpcClient();
    ~RpcClient();

    RpcClient(const RpcClient&) = delete;
    RpcClient& operator=(const RpcClient&) = delete;

    bool connect(const char* addr, uint16_t port, int timeout_ms = DEFAULT_TIMEOUT_MS);  // 连接服务端
    void disconnect();                   // 断开连接
    
    // 同步RPC调用
    bool call(uint32_t func_id, const void* req, size_t req_len,
              void* resp, size_t& resp_len, int timeout_ms = DEFAULT_TIMEOUT_MS);

    bool connected() const { return conn_ && conn_->connected(); }  // 检查连接状态

private:
    bool waitForResponse(uint32_t seq_id, Buffer& recv_buf, int timeout_ms);  // 等待响应

    Transport transport_;                              // RDMA传输层
    std::unique_ptr<Connection> conn_;                 // 连接对象
    std::atomic<uint32_t> next_seq_id_{1};             // 下一个序列号
};

}
#endif
