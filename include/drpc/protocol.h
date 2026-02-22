#ifndef DRPC_PROTOCOL_H
#define DRPC_PROTOCOL_H

#include <drpc/common.h>
#include <drpc/buffer.h>

namespace drpc {

#pragma pack(push, 1)
// RPC消息头，固定20字节
struct MsgHeader {
    uint16_t magic;          // 魔数，用于校验 (0x5250 = "RP")
    uint8_t version;         // 协议版本
    uint8_t type;            // 消息类型
    uint8_t flags;           // 标志位
    uint8_t reserved[3];     // 保留字段
    uint32_t seq_id;         // 序列号，用于匹配请求和响应
    uint32_t func_id;        // 函数ID，标识RPC方法
    uint32_t payload_len;    // 载荷长度
};

// DATA_READY消息体，用于Pull模式通知远端数据就绪
struct DataReadyMsg {
    uint64_t remote_addr;    // 远端数据地址
    uint32_t rkey;           // 远端访问key
    uint32_t data_len;       // 数据长度
    uint32_t seq_id;         // 对应的请求序列号
};
#pragma pack(pop)

// 协议常量
constexpr uint16_t MSG_MAGIC = 0x5250;                    // 魔数
constexpr size_t MSG_HEADER_SIZE = sizeof(MsgHeader);     // 消息头大小
constexpr size_t DATA_READY_MSG_SIZE = sizeof(DataReadyMsg);  // DataReady消息大小

// 消息类型
enum MsgType : uint8_t {
    MSG_REQUEST = 0,        // RPC请求
    MSG_RESPONSE = 1,       // RPC响应
    MSG_ACK = 2,            // 确认消息
    MSG_ERROR = 3,          // 错误消息
    MSG_DATA_READY = 4      // 数据就绪通知(Pull模式)
};

// 消息标志
enum MsgFlag : uint8_t {
    FLAG_NONE = 0,
    FLAG_INLINE = 1 << 0,       // 使用inline发送
    FLAG_HAS_PAYLOAD = 1 << 1,  // 包含载荷
    FLAG_PULL_MODE = 1 << 2     // 使用Pull模式
};

// 获取消息头指针
inline MsgHeader* getMsgHeader(void* data) {
    return reinterpret_cast<MsgHeader*>(data);
}

inline const MsgHeader* getMsgHeader(const void* data) {
    return reinterpret_cast<const MsgHeader*>(data);
}

// 获取载荷指针
inline void* getMsgPayload(void* data) {
    return static_cast<char*>(data) + MSG_HEADER_SIZE;
}

inline const void* getMsgPayload(const void* data) {
    return static_cast<const char*>(data) + MSG_HEADER_SIZE;
}

// 校验消息头
inline bool validateHeader(const MsgHeader* hdr) {
    return hdr && hdr->magic == MSG_MAGIC;
}

// 计算消息总大小
inline size_t msgTotalSize(const MsgHeader* hdr) {
    return MSG_HEADER_SIZE + hdr->payload_len;
}

// 编码RPC请求消息
Buffer encodeRequest(MemoryPool& pool, uint32_t seq_id, uint32_t func_id,
                     const void* payload, size_t payload_len);

// 编码RPC响应消息
Buffer encodeResponse(MemoryPool& pool, uint32_t seq_id,
                      const void* payload, size_t payload_len);

// 编码DATA_READY消息(Pull模式)
Buffer encodeDataReady(MemoryPool& pool, uint32_t seq_id,
                       uint64_t remote_addr, uint32_t rkey, uint32_t data_len);

// 编码ACK确认消息
Buffer encodeAck(MemoryPool& pool, uint32_t seq_id);

}
#endif
