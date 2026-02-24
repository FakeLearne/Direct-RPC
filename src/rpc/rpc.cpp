#include <drpc/rpc.h>
#include <arpa/inet.h>
#include <poll.h>
#include <rdma/rdma_cma.h>

namespace drpc {

RpcServer::RpcServer(const char* addr, uint16_t port)
    : addr_(addr), port_(port) {
}

RpcServer::~RpcServer() {
    stop();
}

// 注册RPC处理函数
void RpcServer::registerHandler(uint32_t func_id, RpcHandler handler) {
    handlers_[func_id] = handler;
}

// 启动服务：初始化传输层、绑定地址、开始监听
ErrorCode RpcServer::start() {
    ErrorCode err = transport_.init();
    if (err != ErrorCode::OK) return err;
    
    // 创建事件通道
    rdma_event_channel* channel = rdma_create_event_channel();
    if (!channel) return ErrorCode::CREATE_FAILED;
    
    // 创建监听cm_id
    if (rdma_create_id(channel, &listen_id_, nullptr, RDMA_PS_TCP)) {
        rdma_destroy_event_channel(channel);
        return ErrorCode::CREATE_FAILED;
    }
    
    // 绑定地址
    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    inet_pton(AF_INET, addr_.c_str(), &server_addr.sin_addr);
    
    if (rdma_bind_addr(listen_id_, (sockaddr*)&server_addr)) {
        rdma_destroy_id(listen_id_);
        rdma_destroy_event_channel(channel);
        listen_id_ = nullptr;
        return ErrorCode::CREATE_FAILED;
    }
    
    // 开始监听
    if (rdma_listen(listen_id_, 10)) {
        rdma_destroy_id(listen_id_);
        rdma_destroy_event_channel(channel);
        listen_id_ = nullptr;
        return ErrorCode::CREATE_FAILED;
    }
    
    running_ = true;
    return ErrorCode::OK;
}

// 停止服务
void RpcServer::stop() {
    running_ = false;
    if (listen_id_) {
        rdma_destroy_id(listen_id_);
        listen_id_ = nullptr;
    }
}

// 运行事件循环，接受连接并处理请求
void RpcServer::run() {
    if (!listen_id_ || !listen_id_->channel) return;
    
    rdma_event_channel* channel = listen_id_->channel;
    
    while (running_) {
        // 等待连接事件
        pollfd pfd;
        pfd.fd = channel->fd;
        pfd.events = POLLIN;
        
        if (poll(&pfd, 1, 100) <= 0) continue;
        
        rdma_cm_event* event = nullptr;
        if (rdma_get_cm_event(channel, &event)) continue;
        
        // 处理连接请求
        if (event->event == RDMA_CM_EVENT_CONNECT_REQUEST) {
            Connection conn(transport_);
            ErrorCode err = conn.accept(event->id, DEFAULT_TIMEOUT_MS);
            if (err == ErrorCode::OK) {
                handleClient(conn);
            }
        }
        
        rdma_ack_cm_event(event);
    }
}

// 处理客户端连接
void RpcServer::handleClient(Connection& conn) {
    Buffer recv_buf = conn.getRecvBuffer();
    if (!recv_buf.valid()) return;
    
    while (running_ && conn.connected()) {
        // 投递接收缓冲区
        ErrorCode err = conn.recv(recv_buf, 1);
        if (err != ErrorCode::OK) break;
        
        // 等待接收完成
        ibv_wc wc;
        err = transport_.pollCompletion(wc, DEFAULT_TIMEOUT_MS);
        if (err != ErrorCode::OK) break;
        
        // 解析消息头
        MsgHeader* hdr = getMsgHeader(recv_buf.addr);
        if (!validateHeader(hdr)) continue;
        
        // 处理请求
        processRequest(conn, hdr);
    }
}

// 处理单个RPC请求
void RpcServer::processRequest(Connection& conn, const MsgHeader* hdr) {
    // 查找处理函数
    auto it = handlers_.find(hdr->func_id);
    if (it == handlers_.end()) return;
    
    // 调用处理函数
    char resp_buf[4096];
    size_t resp_len = sizeof(resp_buf);
    
    it->second(getMsgPayload(hdr), hdr->payload_len, resp_buf, resp_len);
    
    // 编码并发送响应
    Buffer send_buf = encodeResponse(transport_.pool(), hdr->seq_id, resp_buf, resp_len);
    if (!send_buf.valid()) return;
    
    bool use_inline = resp_len <= SMALL_MSG_THRESHOLD;
    ErrorCode err = conn.send(send_buf, MSG_HEADER_SIZE + resp_len, 1, use_inline);
    if (err != ErrorCode::OK) {
        transport_.pool().deallocate(send_buf);
        return;
    }
    
    // 等待发送完成
    ibv_wc wc;
    transport_.pollCompletion(wc, DEFAULT_TIMEOUT_MS);
    
    transport_.pool().deallocate(send_buf);
}

RpcClient::RpcClient() {
}

RpcClient::~RpcClient() {
    disconnect();
}

// 连接服务端
ErrorCode RpcClient::connect(const char* addr, uint16_t port, int timeout_ms) {
    if (!addr) return ErrorCode::INVALID_PARAM;
    
    ErrorCode err = transport_.init();
    if (err != ErrorCode::OK) return err;
    
    conn_ = std::make_unique<Connection>(transport_);
    err = conn_->connect(addr, port, timeout_ms);
    if (err != ErrorCode::OK) {
        conn_.reset();
        return err;
    }
    return ErrorCode::OK;
}

// 断开连接
void RpcClient::disconnect() {
    if (conn_) {
        conn_->close();
        conn_.reset();
    }
}

// 同步RPC调用（Push模式）
ErrorCode RpcClient::call(uint32_t func_id, const void* req, size_t req_len,
                         void* resp, size_t& resp_len, int timeout_ms) {
    if (!connected()) return ErrorCode::DISCONNECTED;
    if (!req && req_len > 0) return ErrorCode::INVALID_PARAM;
    
    uint32_t seq_id = next_seq_id_.fetch_add(1);
    
    // 编码请求
    Buffer send_buf = encodeRequest(transport_.pool(), seq_id, func_id, req, req_len);
    if (!send_buf.valid()) return ErrorCode::NO_MEMORY;
    
    // 分配接收缓冲区
    Buffer recv_buf = transport_.pool().allocate(4096);
    if (!recv_buf.valid()) {
        transport_.pool().deallocate(send_buf);
        return ErrorCode::NO_MEMORY;
    }
    
    // 投递接收缓冲区
    ErrorCode err = conn_->recv(recv_buf, 2);
    if (err != ErrorCode::OK) {
        transport_.pool().deallocate(send_buf);
        transport_.pool().deallocate(recv_buf);
        return err;
    }
    
    // 发送请求
    bool use_inline = (MSG_HEADER_SIZE + req_len) <= SMALL_MSG_THRESHOLD;
    err = conn_->send(send_buf, MSG_HEADER_SIZE + req_len, 1, use_inline);
    if (err != ErrorCode::OK) {
        transport_.pool().deallocate(send_buf);
        transport_.pool().deallocate(recv_buf);
        return err;
    }
    
    // 等待响应
    err = waitForResponse(seq_id, recv_buf, timeout_ms);
    if (err != ErrorCode::OK) {
        transport_.pool().deallocate(send_buf);
        transport_.pool().deallocate(recv_buf);
        return err;
    }
    
    // 解析响应
    MsgHeader* hdr = getMsgHeader(recv_buf.addr);
    if (!validateHeader(hdr) || hdr->type != MSG_RESPONSE) {
        transport_.pool().deallocate(send_buf);
        transport_.pool().deallocate(recv_buf);
        return ErrorCode::ERROR;
    }
    
    // 拷贝响应数据
    size_t copy_len = std::min(resp_len, (size_t)hdr->payload_len);
    if (resp && resp_len > 0) {
        std::memcpy(resp, getMsgPayload(recv_buf.addr), copy_len);
    }
    resp_len = copy_len;
    
    transport_.pool().deallocate(send_buf);
    transport_.pool().deallocate(recv_buf);
    return ErrorCode::OK;
}

// Pull模式RPC调用
ErrorCode RpcClient::callWithPull(uint32_t func_id, const void* req, size_t req_len,
                                  void* resp, size_t& resp_len, int timeout_ms) {
    if (!connected()) return ErrorCode::DISCONNECTED;
    
    // Pull模式流程：
    // 1. Client发送请求
    // 2. Server处理请求，将结果放入预注册内存
    // 3. Server发送DATA_READY消息（包含addr+rkey）
    // 4. Client使用RDMA Read拉取数据
    
    uint32_t seq_id = next_seq_id_.fetch_add(1);
    
    // 编码请求（标记Pull模式）
    Buffer send_buf = encodeRequest(transport_.pool(), seq_id, func_id, req, req_len);
    if (!send_buf.valid()) return ErrorCode::NO_MEMORY;
    
    // 分配接收缓冲区（用于接收DATA_READY）
    Buffer recv_buf = transport_.pool().allocate(4096);
    if (!recv_buf.valid()) {
        transport_.pool().deallocate(send_buf);
        return ErrorCode::NO_MEMORY;
    }
    
    // 分配数据缓冲区（用于RDMA Read）
    Buffer data_buf = transport_.pool().allocate(65536);
    if (!data_buf.valid()) {
        transport_.pool().deallocate(send_buf);
        transport_.pool().deallocate(recv_buf);
        return ErrorCode::NO_MEMORY;
    }
    
    // 投递接收缓冲区
    ErrorCode err = conn_->recv(recv_buf, 2);
    if (err != ErrorCode::OK) {
        transport_.pool().deallocate(send_buf);
        transport_.pool().deallocate(recv_buf);
        transport_.pool().deallocate(data_buf);
        return err;
    }
    
    // 发送请求（告诉Server我们的数据缓冲区地址）
    // 在Pull模式下，我们需要先告诉Server我们的缓冲区信息
    conn_->setRemoteInfo(data_buf.rkey, (uint64_t)data_buf.addr);
    
    bool use_inline = (MSG_HEADER_SIZE + req_len) <= SMALL_MSG_THRESHOLD;
    err = conn_->send(send_buf, MSG_HEADER_SIZE + req_len, 1, use_inline);
    if (err != ErrorCode::OK) {
        transport_.pool().deallocate(send_buf);
        transport_.pool().deallocate(recv_buf);
        transport_.pool().deallocate(data_buf);
        return err;
    }
    
    // 等待响应
    err = waitForResponse(seq_id, recv_buf, timeout_ms);
    if (err != ErrorCode::OK) {
        transport_.pool().deallocate(send_buf);
        transport_.pool().deallocate(recv_buf);
        transport_.pool().deallocate(data_buf);
        return err;
    }
    
    // 解析响应
    MsgHeader* hdr = getMsgHeader(recv_buf.addr);
    if (!validateHeader(hdr)) {
        transport_.pool().deallocate(send_buf);
        transport_.pool().deallocate(recv_buf);
        transport_.pool().deallocate(data_buf);
        return ErrorCode::ERROR;
    }
    
    // 如果是DATA_READY消息，使用RDMA Read拉取数据
    if (hdr->type == MSG_DATA_READY) {
        DataReadyMsg* info = (DataReadyMsg*)getMsgPayload(recv_buf.addr);
        
        // 使用RDMA Read拉取数据
        err = conn_->read(data_buf, info->data_len, 3);
        if (err != ErrorCode::OK) {
            transport_.pool().deallocate(send_buf);
            transport_.pool().deallocate(recv_buf);
            transport_.pool().deallocate(data_buf);
            return err;
        }
        
        // 等待RDMA Read完成
        ibv_wc wc;
        err = transport_.pollCompletion(wc, timeout_ms);
        if (err != ErrorCode::OK) {
            transport_.pool().deallocate(send_buf);
            transport_.pool().deallocate(recv_buf);
            transport_.pool().deallocate(data_buf);
            return err;
        }
        
        // 拷贝数据
        size_t copy_len = std::min(resp_len, (size_t)info->data_len);
        if (resp && resp_len > 0) {
            std::memcpy(resp, data_buf.addr, copy_len);
        }
        resp_len = copy_len;
    } else if (hdr->type == MSG_RESPONSE) {
        // 如果是普通响应，直接拷贝
        size_t copy_len = std::min(resp_len, (size_t)hdr->payload_len);
        if (resp && resp_len > 0) {
            std::memcpy(resp, getMsgPayload(recv_buf.addr), copy_len);
        }
        resp_len = copy_len;
    } else {
        transport_.pool().deallocate(send_buf);
        transport_.pool().deallocate(recv_buf);
        transport_.pool().deallocate(data_buf);
        return ErrorCode::ERROR;
    }
    
    transport_.pool().deallocate(send_buf);
    transport_.pool().deallocate(recv_buf);
    transport_.pool().deallocate(data_buf);
    return ErrorCode::OK;
}

// 等待响应
ErrorCode RpcClient::waitForResponse(uint32_t seq_id, Buffer& recv_buf, int timeout_ms) {
    auto start = std::chrono::steady_clock::now();
    
    while (true) {
        ibv_wc wc;
        ErrorCode err = transport_.pollCompletion(wc, 10);
        
        if (err == ErrorCode::OK && wc.wr_id == 2) {  // recv wr_id
            MsgHeader* hdr = getMsgHeader(recv_buf.addr);
            if (hdr->seq_id == seq_id) {
                return ErrorCode::OK;
            }
        }
        
        // 检查超时
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        if (elapsed.count() >= timeout_ms) {
            return ErrorCode::TIMEOUT;
        }
    }
}

// 等待发送完成
ErrorCode RpcClient::waitForSendComplete(int timeout_ms) {
    ibv_wc wc;
    return transport_.pollCompletion(wc, timeout_ms);
}

}
