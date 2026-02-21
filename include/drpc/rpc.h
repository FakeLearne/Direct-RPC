#ifndef DRPC_RPC_H
#define DRPC_RPC_H

#include <drpc/common.h>
#include <drpc/transport.h>
#include <drpc/buffer.h>
#include <drpc/connection.h>
#include <drpc/protocol.h>
#include <drpc/dispatcher.h>

namespace drpc {

using RPCHandler = std::function<void(const void* req, size_t req_len,
                                       void* resp, size_t& resp_len)>;

class RpcServer {
public:
    RpcServer(const char* addr, uint16_t port);
    ~RpcServer();

    RpcServer(const RpcServer&) = delete;
    RpcServer& operator=(const RpcServer&) = delete;

    void registerHandler(uint32_t func_id, RPCHandler handler);

    Status start();
    void stop();
    bool running() const { return running_.load(); }

    void setThreadCount(int count) { thread_count_ = count; }
    void setMaxConnections(int count) { max_connections_ = count; }

    size_t connectionCount() const;
    uint64_t requestCount() const { return request_count_.load(); }

private:
    void acceptLoop();
    void handleConnection(std::unique_ptr<Connection> conn);
    void handleRequest(Connection& conn, const MessageHeader& hdr, const void* data);

    std::string bind_addr_;
    uint16_t bind_port_;

    std::unique_ptr<RdmaContext> context_;
    std::unique_ptr<ProtectionDomain> pd_;
    std::unique_ptr<CompletionQueue> send_cq_;
    std::unique_ptr<CompletionQueue> recv_cq_;
    std::unique_ptr<MemoryPool> pool_;
    std::unique_ptr<ConnectionManager> cm_;

    std::unordered_map<uint32_t, RPCHandler> handlers_;
    std::vector<std::unique_ptr<Connection>> connections_;
    mutable std::mutex connections_mutex_;

    std::atomic<bool> running_{false};
    std::atomic<uint64_t> request_count_{0};
    std::thread accept_thread_;
    std::vector<std::thread> worker_threads_;

    int thread_count_ = 1;
    int max_connections_ = 100;
};

enum class CallMode {
    SYNC,
    ASYNC,
    PULL
};

class RpcClient {
public:
    RpcClient();
    ~RpcClient();

    RpcClient(const RpcClient&) = delete;
    RpcClient& operator=(const RpcClient&) = delete;

    Status connect(const char* addr, uint16_t port, int timeout_ms = DEFAULT_TIMEOUT_MS);
    void disconnect();
    bool connected() const { return connected_.load(); }

    Buffer call(uint32_t func_id, const void* req, size_t req_len,
                int timeout_ms = DEFAULT_TIMEOUT_MS);

    Buffer callWithPull(uint32_t func_id, const void* req, size_t req_len,
                        int timeout_ms = DEFAULT_TIMEOUT_MS);

    uint32_t asyncCall(uint32_t func_id, const void* req, size_t req_len,
                       std::function<void(Buffer&)> callback);

    bool wait(uint32_t seq_id, int timeout_ms = DEFAULT_TIMEOUT_MS);
    bool waitAll(int timeout_ms = DEFAULT_TIMEOUT_MS);

    void setCallMode(CallMode mode) { default_call_mode_ = mode; }

private:
    Buffer doSyncCall(uint32_t func_id, const void* req, size_t req_len,
                      int timeout_ms, CallMode mode);
    void handleDataReady(const DataReadyInfo& info, Buffer& recv_buf, uint32_t seq_id);

    std::unique_ptr<RdmaContext> context_;
    std::unique_ptr<ProtectionDomain> pd_;
    std::unique_ptr<CompletionQueue> send_cq_;
    std::unique_ptr<CompletionQueue> recv_cq_;
    std::unique_ptr<MemoryPool> pool_;
    std::unique_ptr<Connection> connection_;
    std::unique_ptr<Transport> transport_;
    std::unique_ptr<Dispatcher> dispatcher_;
    std::unique_ptr<RequestContextManager> ctx_manager_;

    std::atomic<uint32_t> next_seq_id_{1};
    std::atomic<bool> connected_{false};
    CallMode default_call_mode_ = CallMode::SYNC;
};

}
#endif
