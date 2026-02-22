#include <drpc/protocol.h>
#include <cstring>

namespace drpc {

// 编码RPC请求消息
Buffer encodeRequest(MemoryPool& pool, uint32_t seq_id, uint32_t func_id,
                     const void* payload, size_t payload_len) {
    size_t total = MSG_HEADER_SIZE + payload_len;
    Buffer buf = pool.allocate(total);
    if (!buf.valid()) return buf;
    
    // 填充消息头
    MsgHeader* hdr = getMsgHeader(buf.addr);
    hdr->magic = MSG_MAGIC;
    hdr->version = 1;
    hdr->type = MSG_REQUEST;
    hdr->flags = payload_len > 0 ? FLAG_HAS_PAYLOAD : FLAG_NONE;
    hdr->seq_id = seq_id;
    hdr->func_id = func_id;
    hdr->payload_len = payload_len;
    
    // 拷贝载荷
    if (payload && payload_len > 0) {
        std::memcpy(getMsgPayload(buf.addr), payload, payload_len);
    }
    
    return buf;
}

// 编码RPC响应消息
Buffer encodeResponse(MemoryPool& pool, uint32_t seq_id,
                      const void* payload, size_t payload_len) {
    size_t total = MSG_HEADER_SIZE + payload_len;
    Buffer buf = pool.allocate(total);
    if (!buf.valid()) return buf;
    
    // 填充消息头
    MsgHeader* hdr = getMsgHeader(buf.addr);
    hdr->magic = MSG_MAGIC;
    hdr->version = 1;
    hdr->type = MSG_RESPONSE;
    hdr->flags = payload_len > 0 ? FLAG_HAS_PAYLOAD : FLAG_NONE;
    hdr->seq_id = seq_id;
    hdr->func_id = 0;
    hdr->payload_len = payload_len;
    
    // 拷贝载荷
    if (payload && payload_len > 0) {
        std::memcpy(getMsgPayload(buf.addr), payload, payload_len);
    }
    
    return buf;
}

// 编码DATA_READY消息，用于Pull模式通知远端数据就绪
Buffer encodeDataReady(MemoryPool& pool, uint32_t seq_id,
                       uint64_t remote_addr, uint32_t rkey, uint32_t data_len) {
    size_t total = MSG_HEADER_SIZE + DATA_READY_MSG_SIZE;
    Buffer buf = pool.allocate(total);
    if (!buf.valid()) return buf;
    
    // 填充消息头
    MsgHeader* hdr = getMsgHeader(buf.addr);
    hdr->magic = MSG_MAGIC;
    hdr->version = 1;
    hdr->type = MSG_DATA_READY;
    hdr->flags = FLAG_HAS_PAYLOAD | FLAG_PULL_MODE;
    hdr->seq_id = seq_id;
    hdr->func_id = 0;
    hdr->payload_len = DATA_READY_MSG_SIZE;
    
    // 填充DataReady消息体
    DataReadyMsg* msg = static_cast<DataReadyMsg*>(getMsgPayload(buf.addr));
    msg->remote_addr = remote_addr;
    msg->rkey = rkey;
    msg->data_len = data_len;
    msg->seq_id = seq_id;
    
    return buf;
}

// 编码ACK确认消息
Buffer encodeAck(MemoryPool& pool, uint32_t seq_id) {
    Buffer buf = pool.allocate(MSG_HEADER_SIZE);
    if (!buf.valid()) return buf;
    
    // 填充消息头
    MsgHeader* hdr = getMsgHeader(buf.addr);
    hdr->magic = MSG_MAGIC;
    hdr->version = 1;
    hdr->type = MSG_ACK;
    hdr->flags = FLAG_NONE;
    hdr->seq_id = seq_id;
    hdr->func_id = 0;
    hdr->payload_len = 0;
    
    return buf;
}

}
