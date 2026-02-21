#ifndef DRPC_PROTOCOL_H
#define DRPC_PROTOCOL_H

#include <drpc/common.h>
#include <drpc/buffer.h>

namespace drpc {

#pragma pack(push, 1)

struct MessageHeader {
    uint16_t magic;
    uint8_t version;
    uint8_t type;
    uint8_t flags;
    uint8_t reserved[3];
    uint32_t seq_id;
    uint32_t func_id;
    uint32_t payload_len;
};

struct DataReadyInfo {
    uint64_t remote_addr;
    uint32_t rkey;
    uint32_t data_len;
    uint32_t seq_id;
};

#pragma pack(pop)

enum class MessageType : uint8_t {
    REQUEST = 0,
    RESPONSE = 1,
    ACK = 2,
    ERROR = 3,
    DATA_READY = 4,
    READ_COMPLETE = 5
};

enum class MessageFlag : uint8_t {
    NONE = 0,
    INLINE = 1 << 0,
    HAS_PAYLOAD = 1 << 1,
    LAST = 1 << 2,
    PULL_MODE = 1 << 3
};

constexpr size_t MESSAGE_HEADER_SIZE = sizeof(MessageHeader);
constexpr size_t DATA_READY_INFO_SIZE = sizeof(DataReadyInfo);

class MessageCodec {
public:
    static Buffer encodeRequest(MemoryPool& pool, uint32_t seq_id,
                                uint32_t func_id, const void* header,
                                size_t header_len, const void* payload,
                                size_t payload_len);

    static Buffer encodeResponse(MemoryPool& pool, uint32_t seq_id,
                                 const void* payload, size_t payload_len,
                                 bool pull_mode = false);

    static Buffer encodeDataReady(MemoryPool& pool, uint32_t seq_id,
                                  uint64_t remote_addr, uint32_t rkey,
                                  uint32_t data_len);

    static Buffer encodeAck(MemoryPool& pool, uint32_t seq_id);

    static Buffer encodeError(MemoryPool& pool, uint32_t seq_id,
                              ErrorCode error_code);

    static bool decodeHeader(const void* data, size_t len, MessageHeader& hdr);
    static bool decodeDataReadyInfo(const void* data, size_t len, DataReadyInfo& info);

    static size_t messageSize(const MessageHeader& hdr);
    static bool validateHeader(const MessageHeader& hdr);
};

class MessageParser {
public:
    explicit MessageParser(Buffer& buf);

    const MessageHeader* header() const;
    const void* headerData() const;
    const void* payload() const;
    size_t payloadSize() const;

    bool validate() const;
    bool hasPayload() const;
    bool isPullMode() const;

    const DataReadyInfo* dataReadyInfo() const;

private:
    Buffer& buffer_;
    const MessageHeader* header_;
};

}
#endif
