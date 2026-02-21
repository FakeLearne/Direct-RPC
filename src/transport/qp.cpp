#include <drpc/transport.h>

namespace drpc {

QueuePair::QueuePair(ProtectionDomain& pd, CompletionQueue& send_cq,
                     CompletionQueue& recv_cq, int max_send_wr, int max_recv_wr) {
}

QueuePair::~QueuePair() {
}

QueuePair::QueuePair(QueuePair&& other) noexcept {
}

QueuePair& QueuePair::operator=(QueuePair&& other) noexcept {
    return *this;
}

Status QueuePair::toInit(uint16_t pkey_index, uint8_t port_num) {
    return Status::OK();
}

Status QueuePair::toRTR(uint32_t remote_qpn, uint16_t dlid, ibv_mtu mtu,
                        uint32_t rq_psn, uint8_t port_num) {
    return Status::OK();
}

Status QueuePair::toRTS(uint16_t retry_cnt, uint16_t rnr_retry, uint16_t sq_psn) {
    return Status::OK();
}

Status QueuePair::toError() {
    return Status::OK();
}

}
