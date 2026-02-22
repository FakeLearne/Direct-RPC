#include <drpc/transport.h>
#include <cstring>
#include <poll.h>

namespace drpc {

Transport::Transport() {}

// 析构函数：按序释放RDMA资源
Transport::~Transport() {
    if (pool_) pool_.reset();
    if (cq_) ibv_destroy_cq(cq_);
    if (pd_) ibv_dealloc_pd(pd_);
    if (ctx_) ibv_close_device(ctx_);
}

// 初始化：打开设备、分配PD、创建CQ、创建内存池
bool Transport::init() {
    int num_devices;
    ibv_device** dev_list = ibv_get_device_list(&num_devices);
    if (!dev_list || num_devices == 0) return false;
    
    // 打开第一个RDMA设备
    ctx_ = ibv_open_device(dev_list[0]);
    ibv_free_device_list(dev_list);
    if (!ctx_) return false;
    
    // 分配保护域
    pd_ = ibv_alloc_pd(ctx_);
    if (!pd_) return false;
    
    // 创建完成队列
    cq_ = ibv_create_cq(ctx_, DEFAULT_CQ_SIZE, nullptr, nullptr, 0);
    if (!cq_) return false;
    
    // 创建内存池
    pool_ = std::make_unique<MemoryPool>(pd_);
    return true;
}

// 轮询完成事件，支持超时
int Transport::pollCompletion(ibv_wc& wc, int timeout_ms) {
    if (timeout_ms > 0) {
        struct pollfd pfd;
        pfd.fd = cq_->channel ? cq_->channel->fd : -1;
        pfd.events = POLLIN;
        
        if (poll(&pfd, 1, timeout_ms) <= 0) {
            return -1;
        }
    }
    
    int ret = ibv_poll_cq(cq_, 1, &wc);
    return ret;
}

// 提交Send操作
int Transport::postSend(ibv_qp* qp, const Buffer& buf, size_t len, 
                        uint64_t wr_id, bool inline_flag) {
    ibv_send_wr wr, *bad_wr = nullptr;
    ibv_sge sge;
    
    std::memset(&wr, 0, sizeof(wr));
    wr.wr_id = wr_id;
    wr.opcode = IBV_WR_SEND;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.send_flags = inline_flag ? IBV_SEND_INLINE : 0;
    
    sge.addr = (uint64_t)buf.addr;
    sge.length = len;
    sge.lkey = buf.lkey;
    
    return ibv_post_send(qp, &wr, &bad_wr);
}

// 提交Send with Immediate操作
int Transport::postSendImm(ibv_qp* qp, const Buffer& buf, size_t len,
                           uint64_t wr_id, uint32_t imm_data) {
    ibv_send_wr wr, *bad_wr = nullptr;
    ibv_sge sge;
    
    std::memset(&wr, 0, sizeof(wr));
    wr.wr_id = wr_id;
    wr.opcode = IBV_WR_SEND_WITH_IMM;
    wr.imm_data = imm_data;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    
    sge.addr = (uint64_t)buf.addr;
    sge.length = len;
    sge.lkey = buf.lkey;
    
    return ibv_post_send(qp, &wr, &bad_wr);
}

// 提交Recv操作
int Transport::postRecv(ibv_qp* qp, const Buffer& buf, uint64_t wr_id) {
    ibv_recv_wr wr, *bad_wr = nullptr;
    ibv_sge sge;
    
    std::memset(&wr, 0, sizeof(wr));
    wr.wr_id = wr_id;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    
    sge.addr = (uint64_t)buf.addr;
    sge.length = buf.size;
    sge.lkey = buf.lkey;
    
    return ibv_post_recv(qp, &wr, &bad_wr);
}

// 提交RDMA Write操作
int Transport::postWrite(ibv_qp* qp, const Buffer& buf, size_t len, uint64_t wr_id,
                         uint32_t remote_rkey, uint64_t remote_addr, bool inline_flag) {
    ibv_send_wr wr, *bad_wr = nullptr;
    ibv_sge sge;
    
    std::memset(&wr, 0, sizeof(wr));
    wr.wr_id = wr_id;
    wr.opcode = IBV_WR_RDMA_WRITE;
    wr.wr.rdma.remote_addr = remote_addr;
    wr.wr.rdma.rkey = remote_rkey;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.send_flags = inline_flag ? IBV_SEND_INLINE : 0;
    
    sge.addr = (uint64_t)buf.addr;
    sge.length = len;
    sge.lkey = buf.lkey;
    
    return ibv_post_send(qp, &wr, &bad_wr);
}

// 提交RDMA Write with Immediate操作
int Transport::postWriteImm(ibv_qp* qp, const Buffer& buf, size_t len, uint64_t wr_id,
                            uint32_t remote_rkey, uint64_t remote_addr, uint32_t imm_data) {
    ibv_send_wr wr, *bad_wr = nullptr;
    ibv_sge sge;
    
    std::memset(&wr, 0, sizeof(wr));
    wr.wr_id = wr_id;
    wr.opcode = IBV_WR_RDMA_WRITE_WITH_IMM;
    wr.wr.rdma.remote_addr = remote_addr;
    wr.wr.rdma.rkey = remote_rkey;
    wr.imm_data = imm_data;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    
    sge.addr = (uint64_t)buf.addr;
    sge.length = len;
    sge.lkey = buf.lkey;
    
    return ibv_post_send(qp, &wr, &bad_wr);
}

// 提交RDMA Read操作
int Transport::postRead(ibv_qp* qp, const Buffer& buf, size_t len, uint64_t wr_id,
                        uint32_t remote_rkey, uint64_t remote_addr) {
    ibv_send_wr wr, *bad_wr = nullptr;
    ibv_sge sge;
    
    std::memset(&wr, 0, sizeof(wr));
    wr.wr_id = wr_id;
    wr.opcode = IBV_WR_RDMA_READ;
    wr.wr.rdma.remote_addr = remote_addr;
    wr.wr.rdma.rkey = remote_rkey;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    
    sge.addr = (uint64_t)buf.addr;
    sge.length = len;
    sge.lkey = buf.lkey;
    
    return ibv_post_send(qp, &wr, &bad_wr);
}

}
