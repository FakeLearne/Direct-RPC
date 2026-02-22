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
bool RpcServer::start() {
    if (!transport_.init()) return false;
    
    // 创建事件通道
    rdma_event_channel* channel = rdma_create_event_channel();
    if (!channel) return false;
    
    // 创建监听cm_id
    if (rdma_create_id(channel, &listen_id_, nullptr, RDMA_PS_TCP)) {
        rdma_destroy_event_channel(channel);
        return false;
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
        return false;
    }
    
    // 开始监听
    if (rdma_listen(listen_id_, 10)) {
        rdma_destroy_id(listen_id_);
        rdma_destroy_event_channel(channel);
        return false;
    }
    
    running_ = true;
    return true;
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
            if (conn.accept(event->id, DEFAULT_TIMEOUT_MS)) {
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
        if (conn.recv(recv_buf, 1) != 0) break;
        
        // 等待接收完成
        ibv_wc wc;
        if (transport_.pollCompletion(wc, DEFAULT_TIMEOUT_MS) <= 0) break;
        if (wc.status != IBV_WC_SUCCESS) break;
        
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
    conn.send(send_buf, MSG_HEADER_SIZE + resp_len, 1, use_inline);
    
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
bool RpcClient::connect(const char* addr, uint16_t port, int timeout_ms) {
    if (!transport_.init()) return false;
    
    conn_ = std::make_unique<Connection>(transport_);
    return conn_->connect(addr, port, timeout_ms);
}

// 断开连接
void RpcClient::disconnect() {
    if (conn_) {
        conn_->close();
        conn_.reset();
    }
}

// 同步RPC调用
bool RpcClient::call(uint32_t func_id, const void* req, size_t req_len,
                     void* resp, size_t& resp_len, int timeout_ms) {
    if (!connected()) return false;
    
    uint32_t seq_id = next_seq_id_.fetch_add(1);
    
    // 编码请求
    Buffer send_buf = encodeRequest(transport_.pool(), seq_id, func_id, req, req_len);
    if (!send_buf.valid()) return false;
    
    // 分配接收缓冲区
    Buffer recv_buf = transport_.pool().allocate(4096);
    if (!recv_buf.valid()) {
        transport_.pool().deallocate(send_buf);
        return false;
    }
    
    // 投递接收缓冲区
    if (conn_->recv(recv_buf, 2) != 0) {
        transport_.pool().deallocate(send_buf);
        transport_.pool().deallocate(recv_buf);
        return false;
    }
    
    // 发送请求
    bool use_inline = (MSG_HEADER_SIZE + req_len) <= SMALL_MSG_THRESHOLD;
    if (conn_->send(send_buf, MSG_HEADER_SIZE + req_len, 1, use_inline) != 0) {
        transport_.pool().deallocate(send_buf);
        transport_.pool().deallocate(recv_buf);
        return false;
    }
    
    // 等待响应
    if (!waitForResponse(seq_id, recv_buf, timeout_ms)) {
        transport_.pool().deallocate(send_buf);
        transport_.pool().deallocate(recv_buf);
        return false;
    }
    
    // 解析响应
    MsgHeader* hdr = getMsgHeader(recv_buf.addr);
    if (!validateHeader(hdr) || hdr->type != MSG_RESPONSE) {
        transport_.pool().deallocate(send_buf);
        transport_.pool().deallocate(recv_buf);
        return false;
    }
    
    // 拷贝响应数据
    size_t copy_len = std::min(resp_len, (size_t)hdr->payload_len);
    std::memcpy(resp, getMsgPayload(recv_buf.addr), copy_len);
    resp_len = copy_len;
    
    transport_.pool().deallocate(send_buf);
    transport_.pool().deallocate(recv_buf);
    return true;
}

// 等待响应，检查序列号匹配
bool RpcClient::waitForResponse(uint32_t seq_id, Buffer& recv_buf, int timeout_ms) {
    auto start = std::chrono::steady_clock::now();
    
    while (true) {
        ibv_wc wc;
        int ret = transport_.pollCompletion(wc, 10);
        
        if (ret > 0 && wc.status == IBV_WC_SUCCESS) {
            if (wc.wr_id == 2) {  // recv wr_id
                MsgHeader* hdr = getMsgHeader(recv_buf.addr);
                if (hdr->seq_id == seq_id) {
                    return true;
                }
            }
        }
        
        // 检查超时
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        if (elapsed.count() >= timeout_ms) {
            return false;
        }
    }
}

}
