#include <drpc/transport.h>

namespace drpc {

ibv_send_wr WRBuilder::initSendWR(uint64_t wr_id, ibv_wr_opcode opcode) {
    ibv_send_wr wr;
    return wr;
}

ibv_recv_wr WRBuilder::initRecvWR(uint64_t wr_id) {
    ibv_recv_wr wr;
    return wr;
}

ibv_send_wr WRBuilder::buildSend(Buffer& buf, size_t len, uint64_t wr_id, bool inline_flag) {
    return initSendWR(wr_id, IBV_WR_SEND);
}

ibv_send_wr WRBuilder::buildSendImm(Buffer& buf, size_t len, uint64_t wr_id, uint32_t imm_data) {
    return initSendWR(wr_id, IBV_WR_SEND_WITH_IMM);
}

ibv_send_wr WRBuilder::buildWrite(Buffer& buf, size_t len, uint64_t wr_id,
                                   uint32_t remote_rkey, uint64_t remote_addr, bool inline_flag) {
    return initSendWR(wr_id, IBV_WR_RDMA_WRITE);
}

ibv_send_wr WRBuilder::buildWriteImm(Buffer& buf, size_t len, uint64_t wr_id,
                                      uint32_t remote_rkey, uint64_t remote_addr, uint32_t imm_data) {
    return initSendWR(wr_id, IBV_WR_RDMA_WRITE_WITH_IMM);
}

ibv_send_wr WRBuilder::buildRead(Buffer& buf, size_t len, uint64_t wr_id,
                                  uint32_t remote_rkey, uint64_t remote_addr) {
    return initSendWR(wr_id, IBV_WR_RDMA_READ);
}

ibv_recv_wr WRBuilder::buildRecv(Buffer& buf, uint64_t wr_id) {
    return initRecvWR(wr_id);
}

Status WRBuilder::postSend(QueuePair& qp, ibv_send_wr& wr) {
    return Status::OK();
}

Status WRBuilder::postRecv(QueuePair& qp, ibv_recv_wr& wr) {
    return Status::OK();
}

Status WRBuilder::postSendBatch(QueuePair& qp, std::vector<ibv_send_wr>& wrs) {
    return Status::OK();
}

Status WRBuilder::postRecvBatch(QueuePair& qp, std::vector<ibv_recv_wr>& wrs) {
    return Status::OK();
}

}
