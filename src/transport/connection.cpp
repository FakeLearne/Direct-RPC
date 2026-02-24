#include <drpc/transport.h>
#include <cstring>
#include <arpa/inet.h>
#include <rdma/rdma_cma.h>

namespace drpc {

Connection::Connection(Transport& transport) : transport_(transport) {
}

Connection::~Connection() {
    close();
}

// 客户端连接：通过rdma_cm建立RDMA连接
ErrorCode Connection::connect(const char* addr, uint16_t port, int timeout_ms) {
    if (!addr) return ErrorCode::INVALID_PARAM;
    
    // 创建事件通道
    rdma_event_channel* channel = rdma_create_event_channel();
    if (!channel) return ErrorCode::CREATE_FAILED;
    
    // 创建cm_id
    if (rdma_create_id(channel, &cm_id_, nullptr, RDMA_PS_TCP)) {
        rdma_destroy_event_channel(channel);
        return ErrorCode::CREATE_FAILED;
    }
    
    // 解析服务器地址
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, addr, &server_addr.sin_addr);
    
    // 地址解析
    if (rdma_resolve_addr(cm_id_, nullptr, (sockaddr*)&server_addr, timeout_ms)) {
        rdma_destroy_id(cm_id_);
        rdma_destroy_event_channel(channel);
        cm_id_ = nullptr;
        return ErrorCode::CONNECT_FAILED;
    }
    
    // 等待地址解析完成
    rdma_cm_event* event = nullptr;
    if (rdma_get_cm_event(channel, &event)) {
        rdma_destroy_id(cm_id_);
        rdma_destroy_event_channel(channel);
        cm_id_ = nullptr;
        return ErrorCode::CONNECT_FAILED;
    }
    
    if (event->event != RDMA_CM_EVENT_ADDR_RESOLVED) {
        rdma_ack_cm_event(event);
        rdma_destroy_id(cm_id_);
        rdma_destroy_event_channel(channel);
        cm_id_ = nullptr;
        return ErrorCode::CONNECT_FAILED;
    }
    rdma_ack_cm_event(event);
    
    // 路由解析
    if (rdma_resolve_route(cm_id_, timeout_ms)) {
        rdma_destroy_id(cm_id_);
        rdma_destroy_event_channel(channel);
        cm_id_ = nullptr;
        return ErrorCode::CONNECT_FAILED;
    }
    
    // 等待路由解析完成
    if (rdma_get_cm_event(channel, &event)) {
        rdma_destroy_id(cm_id_);
        rdma_destroy_event_channel(channel);
        cm_id_ = nullptr;
        return ErrorCode::CONNECT_FAILED;
    }
    
    if (event->event != RDMA_CM_EVENT_ROUTE_RESOLVED) {
        rdma_ack_cm_event(event);
        rdma_destroy_id(cm_id_);
        rdma_destroy_event_channel(channel);
        cm_id_ = nullptr;
        return ErrorCode::CONNECT_FAILED;
    }
    rdma_ack_cm_event(event);
    
    // 创建QP并转换状态
    ErrorCode err = setupQP();
    if (err != ErrorCode::OK) {
        rdma_destroy_id(cm_id_);
        rdma_destroy_event_channel(channel);
        cm_id_ = nullptr;
        return err;
    }
    
    err = transitionToInit();
    if (err != ErrorCode::OK) {
        rdma_destroy_id(cm_id_);
        rdma_destroy_event_channel(channel);
        cm_id_ = nullptr;
        return err;
    }
    
    err = transitionToRTR();
    if (err != ErrorCode::OK) {
        rdma_destroy_id(cm_id_);
        rdma_destroy_event_channel(channel);
        cm_id_ = nullptr;
        return err;
    }
    
    // 发起连接
    rdma_conn_param conn_param;
    std::memset(&conn_param, 0, sizeof(conn_param));
    conn_param.initiator_depth = 1;
    conn_param.responder_resources = 1;
    conn_param.retry_count = DEFAULT_RETRY_COUNT;
    
    if (rdma_connect(cm_id_, &conn_param)) {
        rdma_destroy_id(cm_id_);
        rdma_destroy_event_channel(channel);
        cm_id_ = nullptr;
        return ErrorCode::CONNECT_FAILED;
    }
    
    // 等待连接建立
    if (rdma_get_cm_event(channel, &event)) {
        rdma_destroy_id(cm_id_);
        rdma_destroy_event_channel(channel);
        cm_id_ = nullptr;
        return ErrorCode::CONNECT_FAILED;
    }
    
    if (event->event != RDMA_CM_EVENT_ESTABLISHED) {
        rdma_ack_cm_event(event);
        rdma_destroy_id(cm_id_);
        rdma_destroy_event_channel(channel);
        cm_id_ = nullptr;
        return ErrorCode::CONNECT_FAILED;
    }
    
    rdma_ack_cm_event(event);
    rdma_destroy_event_channel(channel);
    
    // 转换到RTS状态
    err = transitionToRTS();
    if (err != ErrorCode::OK) {
        rdma_disconnect(cm_id_);
        rdma_destroy_id(cm_id_);
        cm_id_ = nullptr;
        return err;
    }
    
    connected_ = true;
    is_server_ = false;
    return ErrorCode::OK;
}

// 服务端接受连接
ErrorCode Connection::accept(rdma_cm_id* listener_id, int timeout_ms) {
    if (!listener_id) return ErrorCode::INVALID_PARAM;
    
    cm_id_ = listener_id;
    
    // 创建QP并转换状态
    ErrorCode err = setupQP();
    if (err != ErrorCode::OK) {
        return err;
    }
    
    err = transitionToInit();
    if (err != ErrorCode::OK) {
        return err;
    }
    
    err = transitionToRTR();
    if (err != ErrorCode::OK) {
        return err;
    }
    
    // 接受连接
    rdma_conn_param conn_param;
    std::memset(&conn_param, 0, sizeof(conn_param));
    conn_param.initiator_depth = 1;
    conn_param.responder_resources = 1;
    
    if (rdma_accept(cm_id_, &conn_param)) {
        return ErrorCode::CONNECT_FAILED;
    }
    
    // 转换到RTS状态
    err = transitionToRTS();
    if (err != ErrorCode::OK) {
        rdma_reject(cm_id_, nullptr, 0);
        return err;
    }
    
    connected_ = true;
    is_server_ = true;
    return ErrorCode::OK;
}

// 关闭连接
void Connection::close() {
    if (connected_) {
        rdma_disconnect(cm_id_);
        connected_ = false;
    }
    if (qp_) {
        ibv_destroy_qp(qp_);
        qp_ = nullptr;
    }
    if (cm_id_ && !is_server_) {
        rdma_destroy_id(cm_id_);
        cm_id_ = nullptr;
    }
}

// 创建QP
ErrorCode Connection::setupQP() {
    ibv_qp_init_attr attr;
    std::memset(&attr, 0, sizeof(attr));
    attr.send_cq = transport_.cq();
    attr.recv_cq = transport_.cq();
    attr.cap.max_send_wr = DEFAULT_QP_DEPTH;
    attr.cap.max_recv_wr = DEFAULT_QP_DEPTH;
    attr.cap.max_send_sge = 1;
    attr.cap.max_recv_sge = 1;
    attr.qp_type = IBV_QPT_RC;
    
    if (rdma_create_qp(cm_id_, transport_.pd(), &attr) != 0) {
        return ErrorCode::CREATE_FAILED;
    }
    qp_ = cm_id_->qp;
    return ErrorCode::OK;
}

// QP状态转换: RESET -> INIT
ErrorCode Connection::transitionToInit() {
    if (!qp_) return ErrorCode::QP_STATE_ERROR;
    
    ibv_qp_attr attr;
    std::memset(&attr, 0, sizeof(attr));
    attr.qp_state = IBV_QPS_INIT;
    attr.pkey_index = 0;
    attr.port_num = 1;
    attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ;
    
    if (ibv_modify_qp(qp_, &attr, 
                      IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS) != 0) {
        return ErrorCode::QP_STATE_ERROR;
    }
    return ErrorCode::OK;
}

// QP状态转换: INIT -> RTR
ErrorCode Connection::transitionToRTR() {
    if (!qp_ || !cm_id_->qp) return ErrorCode::QP_STATE_ERROR;
    
    ibv_qp_attr attr;
    std::memset(&attr, 0, sizeof(attr));
    attr.qp_state = IBV_QPS_RTR;
    attr.path_mtu = IBV_MTU_1024;
    attr.dest_qp_num = cm_id_->qp->qp_num;
    attr.rq_psn = 0;
    attr.max_dest_rd_atomic = 1;
    attr.min_rnr_timer = 12;
    attr.ah_attr.is_global = 0;
    attr.ah_attr.dlid = cm_id_->route.path_rec->dlid;
    attr.ah_attr.sl = 0;
    attr.ah_attr.src_path_bits = 0;
    attr.ah_attr.port_num = 1;
    
    remote_qpn_ = attr.dest_qp_num;
    remote_lid_ = attr.ah_attr.dlid;
    
    if (ibv_modify_qp(qp_, &attr,
                      IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN |
                      IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER) != 0) {
        return ErrorCode::QP_STATE_ERROR;
    }
    return ErrorCode::OK;
}

// QP状态转换: RTR -> RTS
ErrorCode Connection::transitionToRTS() {
    if (!qp_) return ErrorCode::QP_STATE_ERROR;
    
    ibv_qp_attr attr;
    std::memset(&attr, 0, sizeof(attr));
    attr.qp_state = IBV_QPS_RTS;
    attr.sq_psn = 0;
    attr.timeout = 14;
    attr.retry_cnt = DEFAULT_RETRY_COUNT;
    attr.rnr_retry = DEFAULT_RETRY_COUNT;
    attr.max_rd_atomic = 1;
    
    if (ibv_modify_qp(qp_, &attr,
                      IBV_QP_STATE | IBV_QP_SQ_PSN | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT |
                      IBV_QP_RNR_RETRY | IBV_QP_MAX_QP_RD_ATOMIC) != 0) {
        return ErrorCode::QP_STATE_ERROR;
    }
    return ErrorCode::OK;
}

// 获取发送缓冲区
Buffer Connection::getSendBuffer() {
    return transport_.pool().allocate(4096);
}

// 获取接收缓冲区
Buffer Connection::getRecvBuffer() {
    return transport_.pool().allocate(4096);
}

// 归还缓冲区
void Connection::returnBuffer(Buffer& buf) {
    transport_.pool().deallocate(buf);
}

// 发送数据
ErrorCode Connection::send(const Buffer& buf, size_t len, uint64_t wr_id, bool inline_flag) {
    return Transport::postSend(qp_, buf, len, wr_id, inline_flag);
}

// 发送带立即数的数据
ErrorCode Connection::sendImm(const Buffer& buf, size_t len, uint64_t wr_id, uint32_t imm_data) {
    return Transport::postSendImm(qp_, buf, len, wr_id, imm_data);
}

// 投递接收缓冲区
ErrorCode Connection::recv(const Buffer& buf, uint64_t wr_id) {
    return Transport::postRecv(qp_, buf, wr_id);
}

// RDMA Write
ErrorCode Connection::write(const Buffer& buf, size_t len, uint64_t wr_id, bool inline_flag) {
    if (remote_rkey_ == 0) return ErrorCode::INVALID_PARAM;
    return Transport::postWrite(qp_, buf, len, wr_id, remote_rkey_, remote_addr_, inline_flag);
}

// RDMA Write with Immediate
ErrorCode Connection::writeImm(const Buffer& buf, size_t len, uint64_t wr_id, uint32_t imm_data) {
    if (remote_rkey_ == 0) return ErrorCode::INVALID_PARAM;
    return Transport::postWriteImm(qp_, buf, len, wr_id, remote_rkey_, remote_addr_, imm_data);
}

// RDMA Read
ErrorCode Connection::read(const Buffer& buf, size_t len, uint64_t wr_id) {
    if (remote_rkey_ == 0) return ErrorCode::INVALID_PARAM;
    return Transport::postRead(qp_, buf, len, wr_id, remote_rkey_, remote_addr_);
}

}
