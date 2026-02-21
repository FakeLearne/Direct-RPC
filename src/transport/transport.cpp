#include <drpc/transport.h>

namespace drpc {

Transport::Transport(CompletionQueue& send_cq, CompletionQueue& recv_cq)
    : send_cq_(send_cq), recv_cq_(recv_cq) {
}

Status Transport::send(QueuePair& qp, Buffer& buf, size_t len, uint64_t wr_id) {
    return Status::OK();
}

Status Transport::sendInline(QueuePair& qp, Buffer& buf, size_t len, uint64_t wr_id) {
    return Status::OK();
}

Status Transport::sendImm(QueuePair& qp, Buffer& buf, size_t len, uint64_t wr_id, uint32_t imm_data) {
    return Status::OK();
}

Status Transport::postRecv(QueuePair& qp, Buffer& buf, uint64_t wr_id) {
    return Status::OK();
}

Status Transport::write(QueuePair& qp, Buffer& buf, size_t len, uint64_t wr_id,
                        uint32_t remote_rkey, uint64_t remote_addr) {
    return Status::OK();
}

Status Transport::writeInline(QueuePair& qp, Buffer& buf, size_t len, uint64_t wr_id,
                              uint32_t remote_rkey, uint64_t remote_addr) {
    return Status::OK();
}

Status Transport::writeImm(QueuePair& qp, Buffer& buf, size_t len, uint64_t wr_id,
                           uint32_t remote_rkey, uint64_t remote_addr, uint32_t imm_data) {
    return Status::OK();
}

Status Transport::read(QueuePair& qp, Buffer& buf, size_t len, uint64_t wr_id,
                       uint32_t remote_rkey, uint64_t remote_addr) {
    return Status::OK();
}

int Transport::pollSend(std::vector<ibv_wc>& wcs) {
    return send_cq_.poll(wcs);
}

int Transport::pollRecv(std::vector<ibv_wc>& wcs) {
    return recv_cq_.poll(wcs);
}

int Transport::pollAll(std::vector<ibv_wc>& wcs) {
    return 0;
}

}
