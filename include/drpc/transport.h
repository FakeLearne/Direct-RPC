#ifndef DRPC_TRANSPORT_H
#define DRPC_TRANSPORT_H

#include <drpc/common.h>
#include <drpc/buffer.h>
#include <infiniband/verbs.h>

namespace drpc {

class RdmaContext {
public:
    explicit RdmaContext(int device_index = 0);
    ~RdmaContext();

    RdmaContext(const RdmaContext&) = delete;
    RdmaContext& operator=(const RdmaContext&) = delete;
    RdmaContext(RdmaContext&& other) noexcept;
    RdmaContext& operator=(RdmaContext&& other) noexcept;

    bool valid() const { return ctx_ != nullptr; }
    ibv_context* get() const { return ctx_; }
    ibv_device* device() const { return dev_; }
    const char* deviceName() const;

    static int deviceCount();
    static std::vector<std::string> listDevices();

private:
    ibv_device** dev_list_ = nullptr;
    ibv_device* dev_ = nullptr;
    ibv_context* ctx_ = nullptr;
};

class ProtectionDomain {
public:
    explicit ProtectionDomain(RdmaContext& ctx);
    ~ProtectionDomain();

    ProtectionDomain(const ProtectionDomain&) = delete;
    ProtectionDomain& operator=(const ProtectionDomain&) = delete;
    ProtectionDomain(ProtectionDomain&& other) noexcept;
    ProtectionDomain& operator=(ProtectionDomain&& other) noexcept;

    bool valid() const { return pd_ != nullptr; }
    ibv_pd* get() const { return pd_; }

private:
    ibv_pd* pd_ = nullptr;
};

class CompletionQueue {
public:
    CompletionQueue(RdmaContext& ctx, int cqe = DEFAULT_CQ_SIZE);
    ~CompletionQueue();

    CompletionQueue(const CompletionQueue&) = delete;
    CompletionQueue& operator=(const CompletionQueue&) = delete;
    CompletionQueue(CompletionQueue&& other) noexcept;
    CompletionQueue& operator=(CompletionQueue&& other) noexcept;

    bool valid() const { return cq_ != nullptr; }
    ibv_cq* get() const { return cq_; }
    int cqe() const { return cqe_; }

    int poll(std::vector<ibv_wc>& wcs);
    int pollOne(ibv_wc& wc);

private:
    ibv_cq* cq_ = nullptr;
    int cqe_ = 0;
};

enum class QPState { RESET, INIT, RTR, RTS, ERROR };

class QueuePair {
public:
    QueuePair(ProtectionDomain& pd, CompletionQueue& send_cq,
              CompletionQueue& recv_cq, int max_send_wr = DEFAULT_QP_DEPTH,
              int max_recv_wr = DEFAULT_QP_DEPTH);
    ~QueuePair();

    QueuePair(const QueuePair&) = delete;
    QueuePair& operator=(const QueuePair&) = delete;
    QueuePair(QueuePair&& other) noexcept;
    QueuePair& operator=(QueuePair&& other) noexcept;

    bool valid() const { return qp_ != nullptr; }
    ibv_qp* get() const { return qp_; }
    uint32_t qpNum() const { return qp_ ? qp_->qp_num : 0; }
    QPState state() const { return state_; }

    Status toInit(uint16_t pkey_index = 0, uint8_t port_num = 1);
    Status toRTR(uint32_t remote_qpn, uint16_t dlid, ibv_mtu mtu,
                 uint32_t rq_psn = 0, uint8_t port_num = 1);
    Status toRTS(uint16_t retry_cnt = DEFAULT_RETRY_COUNT,
                 uint16_t rnr_retry = DEFAULT_RNR_RETRY,
                 uint16_t sq_psn = 0);
    Status toError();

private:
    ibv_qp* qp_ = nullptr;
    QPState state_ = QPState::RESET;
    int max_send_wr_ = 0;
    int max_recv_wr_ = 0;
};

class WRBuilder {
public:
    static ibv_send_wr buildSend(Buffer& buf, size_t len, uint64_t wr_id,
                                  bool inline_flag = false);
    static ibv_send_wr buildSendImm(Buffer& buf, size_t len, uint64_t wr_id,
                                     uint32_t imm_data);
    static ibv_send_wr buildWrite(Buffer& buf, size_t len, uint64_t wr_id,
                                   uint32_t remote_rkey, uint64_t remote_addr,
                                   bool inline_flag = false);
    static ibv_send_wr buildWriteImm(Buffer& buf, size_t len, uint64_t wr_id,
                                      uint32_t remote_rkey, uint64_t remote_addr,
                                      uint32_t imm_data);
    static ibv_send_wr buildRead(Buffer& buf, size_t len, uint64_t wr_id,
                                  uint32_t remote_rkey, uint64_t remote_addr);

    static ibv_recv_wr buildRecv(Buffer& buf, uint64_t wr_id);

    static Status postSend(QueuePair& qp, ibv_send_wr& wr);
    static Status postRecv(QueuePair& qp, ibv_recv_wr& wr);
    static Status postSendBatch(QueuePair& qp, std::vector<ibv_send_wr>& wrs);
    static Status postRecvBatch(QueuePair& qp, std::vector<ibv_recv_wr>& wrs);

private:
    static ibv_send_wr initSendWR(uint64_t wr_id, ibv_wr_opcode opcode);
    static ibv_recv_wr initRecvWR(uint64_t wr_id);
};

class Connection;

class Transport {
public:
    Transport(CompletionQueue& send_cq, CompletionQueue& recv_cq);
    ~Transport() = default;

    Status send(QueuePair& qp, Buffer& buf, size_t len, uint64_t wr_id);
    Status sendInline(QueuePair& qp, Buffer& buf, size_t len, uint64_t wr_id);
    Status sendImm(QueuePair& qp, Buffer& buf, size_t len, uint64_t wr_id,
                   uint32_t imm_data);
    Status postRecv(QueuePair& qp, Buffer& buf, uint64_t wr_id);

    Status write(QueuePair& qp, Buffer& buf, size_t len, uint64_t wr_id,
                 uint32_t remote_rkey, uint64_t remote_addr);
    Status writeInline(QueuePair& qp, Buffer& buf, size_t len, uint64_t wr_id,
                       uint32_t remote_rkey, uint64_t remote_addr);
    Status writeImm(QueuePair& qp, Buffer& buf, size_t len, uint64_t wr_id,
                    uint32_t remote_rkey, uint64_t remote_addr, uint32_t imm_data);

    Status read(QueuePair& qp, Buffer& buf, size_t len, uint64_t wr_id,
                uint32_t remote_rkey, uint64_t remote_addr);

    int pollSend(std::vector<ibv_wc>& wcs);
    int pollRecv(std::vector<ibv_wc>& wcs);
    int pollAll(std::vector<ibv_wc>& wcs);

    CompletionQueue& sendCQ() { return send_cq_; }
    CompletionQueue& recvCQ() { return recv_cq_; }

private:
    CompletionQueue& send_cq_;
    CompletionQueue& recv_cq_;
};

}
#endif
