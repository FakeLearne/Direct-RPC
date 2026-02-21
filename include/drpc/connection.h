#ifndef DRPC_CONNECTION_H
#define DRPC_CONNECTION_H

#include <drpc/common.h>
#include <drpc/transport.h>
#include <drpc/buffer.h>
#include <rdma/rdma_cma.h>

namespace drpc {

class ConnectionManager {
public:
    explicit ConnectionManager(bool is_server = false);
    ~ConnectionManager();

    ConnectionManager(const ConnectionManager&) = delete;
    ConnectionManager& operator=(const ConnectionManager&) = delete;

    Status bind(const char* addr, uint16_t port);
    Status listen(int backlog = 10);
    std::unique_ptr<ConnectionManager> accept();

    Status connect(const char* addr, uint16_t port, int timeout_ms = DEFAULT_TIMEOUT_MS);
    Status disconnect();

    int getEvent(rdma_cm_event** event);
    void ackEvent(rdma_cm_event* event);

    bool valid() const { return cm_id_ != nullptr; }
    rdma_cm_id* get() const { return cm_id_; }
    bool isServer() const { return is_server_; }
    bool isConnected() const { return connected_; }

    uint16_t localPort() const;
    const char* localAddr() const;

private:
    rdma_event_channel* event_channel_ = nullptr;
    rdma_cm_id* cm_id_ = nullptr;
    bool is_server_ = false;
    bool connected_ = false;
};

class Connection {
public:
    Connection(std::unique_ptr<ConnectionManager> cm,
               ProtectionDomain& pd,
               CompletionQueue& send_cq,
               CompletionQueue& recv_cq,
               MemoryPool& pool);
    ~Connection();

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection(Connection&&) = delete;
    Connection& operator=(Connection&&) = delete;

    bool valid() const;
    bool ready() const;

    QueuePair& qp() { return *qp_; }
    ConnectionBuffer& buffers() { return *buffers_; }
    ConnectionManager& cm() { return *cm_; }

    uint32_t peerQPN() const { return peer_qpn_; }
    uint16_t peerLID() const { return peer_lid_; }

    void setPeerInfo(uint32_t rkey, uint64_t addr);
    uint32_t peerRkey() const { return peer_rkey_; }
    uint64_t peerAddr() const { return peer_addr_; }

    Status close();

private:
    Status setupQP();

    std::unique_ptr<ConnectionManager> cm_;
    std::unique_ptr<QueuePair> qp_;
    std::unique_ptr<ConnectionBuffer> buffers_;

    uint32_t peer_qpn_ = 0;
    uint16_t peer_lid_ = 0;
    uint32_t peer_rkey_ = 0;
    uint64_t peer_addr_ = 0;
};

struct PeerInfo {
    uint32_t rkey;
    uint64_t addr;
    size_t size;
};

}
#endif
