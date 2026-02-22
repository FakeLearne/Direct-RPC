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

    bool init();                        // 初始化RDMA资源
    
    ibv_pd* pd() { return pd_; }        // 获取保护域
    ibv_cq* cq() { return cq_; }        // 获取完成队列
    ibv_context* context() { return ctx_; }  // 获取设备上下文
    MemoryPool& pool() { return *pool_; }    // 获取内存池

    int pollCompletion(ibv_wc& wc, int timeout_ms = 0);  // 轮询完成事件

    // 静态方法：提交各类RDMA操作
    static int postSend(ibv_qp* qp, const Buffer& buf, size_t len, 
                        uint64_t wr_id, bool inline_flag = false);  // 提交Send操作
    static int postSendImm(ibv_qp* qp, const Buffer& buf, size_t len,
                           uint64_t wr_id, uint32_t imm_data);      // 提交Send with Imm操作
    static int postRecv(ibv_qp* qp, const Buffer& buf, uint64_t wr_id);  // 提交Recv操作
    static int postWrite(ibv_qp* qp, const Buffer& buf, size_t len, uint64_t wr_id,
                         uint32_t remote_rkey, uint64_t remote_addr, 
                         bool inline_flag = false);                  // 提交RDMA Write操作
    static int postWriteImm(ibv_qp* qp, const Buffer& buf, size_t len, uint64_t wr_id,
                            uint32_t remote_rkey, uint64_t remote_addr, 
                            uint32_t imm_data);                      // 提交RDMA Write with Imm操作
    static int postRead(ibv_qp* qp, const Buffer& buf, size_t len, uint64_t wr_id,
                        uint32_t remote_rkey, uint64_t remote_addr);  // 提交RDMA Read操作

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

    bool connect(const char* addr, uint16_t port, int timeout_ms = DEFAULT_TIMEOUT_MS);  // 客户端连接
    bool accept(rdma_cm_id* listener_id, int timeout_ms = DEFAULT_TIMEOUT_MS);           // 服务端接受连接
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

    int send(const Buffer& buf, size_t len, uint64_t wr_id, bool inline_flag = false);  // 发送数据
    int sendImm(const Buffer& buf, size_t len, uint64_t wr_id, uint32_t imm_data);      // 发送带立即数
    int recv(const Buffer& buf, uint64_t wr_id);          // 投递接收缓冲区
    int write(const Buffer& buf, size_t len, uint64_t wr_id, bool inline_flag = false); // RDMA Write
    int writeImm(const Buffer& buf, size_t len, uint64_t wr_id, uint32_t imm_data);     // RDMA Write with Imm
    int read(const Buffer& buf, size_t len, uint64_t wr_id);  // RDMA Read

private:
    bool setupQP();                     // 创建QP
    bool transitionToInit();            // QP状态转换: -> INIT
    bool transitionToRTR();             // QP状态转换: INIT -> RTR
    bool transitionToRTS();             // QP状态转换: RTR -> RTS

    Transport& transport_;              // 传输层引用
    rdma_cm_id* cm_id_ = nullptr;       // rdma_cm连接ID
    ibv_qp* qp_ = nullptr;              // 队列对
    bool connected_ = false;            // 连接状态
    bool is_server_ = false;            // 是否为服务端

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
