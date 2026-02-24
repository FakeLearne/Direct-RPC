#ifndef DRPC_TRANSPORT_H
#define DRPC_TRANSPORT_H

#include <drpc/common.h>
#include <drpc/buffer.h>
#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h>

namespace drpc {

// RDMA传输层，管理设备、PD、CQ和内存池
class Transport {
public:
    Transport();
    ~Transport();

    Transport(const Transport&) = delete;
    Transport& operator=(const Transport&) = delete;

    ErrorCode init();                  // 初始化RDMA资源，返回错误码
    
    ibv_pd* pd() { return pd_; }        // 获取保护域
    ibv_cq* cq() { return cq_; }        // 获取完成队列
    ibv_context* context() { return ctx_; }  // 获取设备上下文
    MemoryPool& pool() { return *pool_; }    // 获取内存池

    // 轮询完成事件，返回错误码
    ErrorCode pollCompletion(ibv_wc& wc, int timeout_ms = 0);

    // 静态方法：提交各类RDMA操作，返回错误码
    static ErrorCode postSend(ibv_qp* qp, const Buffer& buf, size_t len, 
                              uint64_t wr_id, bool inline_flag = false);
    static ErrorCode postSendImm(ibv_qp* qp, const Buffer& buf, size_t len,
                                 uint64_t wr_id, uint32_t imm_data);
    static ErrorCode postRecv(ibv_qp* qp, const Buffer& buf, uint64_t wr_id);
    static ErrorCode postWrite(ibv_qp* qp, const Buffer& buf, size_t len, uint64_t wr_id,
                               uint32_t remote_rkey, uint64_t remote_addr, 
                               bool inline_flag = false);
    static ErrorCode postWriteImm(ibv_qp* qp, const Buffer& buf, size_t len, uint64_t wr_id,
                                  uint32_t remote_rkey, uint64_t remote_addr, uint32_t imm_data);
    static ErrorCode postRead(ibv_qp* qp, const Buffer& buf, size_t len, uint64_t wr_id,
                              uint32_t remote_rkey, uint64_t remote_addr);

private:
    ibv_context* ctx_ = nullptr;        // 设备上下文
    ibv_pd* pd_ = nullptr;              // 保护域
    ibv_cq* cq_ = nullptr;              // 完成队列
    std::unique_ptr<MemoryPool> pool_;  // 内存池
};

// RDMA连接，封装QP状态机和rdma_cm操作
class Connection {
public:
    Connection(Transport& transport);
    ~Connection();

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    ErrorCode connect(const char* addr, uint16_t port, int timeout_ms = DEFAULT_TIMEOUT_MS);  // 客户端连接
    ErrorCode accept(rdma_cm_id* listener_id, int timeout_ms = DEFAULT_TIMEOUT_MS);           // 服务端接受连接
    void close();                       // 关闭连接

    bool connected() const { return connected_; }  // 检查连接状态
    ibv_qp* qp() { return qp_; }        // 获取QP
    uint32_t remoteRkey() const { return remote_rkey_; }  // 获取远端rkey
    uint64_t remoteAddr() const { return remote_addr_; }  // 获取远端地址
    void setRemoteInfo(uint32_t rkey, uint64_t addr) {    // 设置远端信息
        remote_rkey_ = rkey;
        remote_addr_ = addr;
    }

    Buffer getSendBuffer();             // 获取发送缓冲区
    Buffer getRecvBuffer();             // 获取接收缓冲区
    void returnBuffer(Buffer& buf);     // 归还缓冲区
    MemoryPool& pool();                 // 获取内存池引用
    ErrorCode pollCompletion(ibv_wc& wc, int timeout_ms = 0);  // 轮询完成事件

    ErrorCode send(const Buffer& buf, size_t len, uint64_t wr_id, bool inline_flag = false);  // 发送数据
    ErrorCode sendImm(const Buffer& buf, size_t len, uint64_t wr_id, uint32_t imm_data);      // 发送带立即数
    ErrorCode recv(const Buffer& buf, uint64_t wr_id);          // 投递接收缓冲区
    ErrorCode write(const Buffer& buf, size_t len, uint64_t wr_id, bool inline_flag = false); // RDMA Write
    ErrorCode writeImm(const Buffer& buf, size_t len, uint64_t wr_id, uint32_t imm_data);     // RDMA Write with Imm
    ErrorCode read(const Buffer& buf, size_t len, uint64_t wr_id);  // RDMA Read

private:
    ErrorCode setupQP();                 // 创建QP
    ErrorCode transitionToInit();        // QP状态转换: -> INIT
    ErrorCode transitionToRTR();         // QP状态转换: INIT -> RTR
    ErrorCode transitionToRTS();         // QP状态转换: RTR -> RTS
    ErrorCode createClientResources();   // 为客户端创建PD、CQ、内存池

    Transport& transport_;              // 传输层引用（服务端使用）
    rdma_cm_id* cm_id_ = nullptr;       // rdma_cm连接ID
    ibv_qp* qp_ = nullptr;              // 队列对
    bool connected_ = false;            // 连接状态
    bool is_server_ = false;            // 是否为服务端

    // 客户端专用资源（在rdma_cm确定设备后创建）
    ibv_pd* client_pd_ = nullptr;       // 客户端保护域
    ibv_cq* client_cq_ = nullptr;       // 客户端完成队列
    std::unique_ptr<MemoryPool> client_pool_;  // 客户端内存池

    uint32_t remote_rkey_ = 0;          // 远端rkey
    uint64_t remote_addr_ = 0;          // 远端地址
    uint16_t remote_lid_ = 0;           // 远端LID
    uint32_t remote_qpn_ = 0;           // 远端QP号

    std::vector<Buffer> send_bufs_;     // 发送缓冲区池
    std::vector<Buffer> recv_bufs_;     // 接收缓冲区池
    size_t send_idx_ = 0;               // 发送缓冲区索引
    size_t recv_idx_ = 0;               // 接收缓冲区索引
};

}
#endif
