#include <drpc/protocol.h>

namespace drpc {

Buffer MessageCodec::encodeRequest(MemoryPool& pool, uint32_t seq_id,
                                   uint32_t func_id, const void* header,
                                   size_t header_len, const void* payload,
                                   size_t payload_len) {
    return Buffer();
}

Buffer MessageCodec::encodeResponse(MemoryPool& pool, uint32_t seq_id,
                                    const void* payload, size_t payload_len,
                                    bool pull_mode) {
    return Buffer();
}

Buffer MessageCodec::encodeDataReady(MemoryPool& pool, uint32_t seq_id,
                                     uint64_t remote_addr, uint32_t rkey,
                                     uint32_t data_len) {
    return Buffer();
}

Buffer MessageCodec::encodeAck(MemoryPool& pool, uint32_t seq_id) {
    return Buffer();
}

Buffer MessageCodec::encodeError(MemoryPool& pool, uint32_t seq_id,
                                 ErrorCode error_code) {
    return Buffer();
}

bool MessageCodec::decodeHeader(const void* data, size_t len, MessageHeader& hdr) {
    return false;
}

bool MessageCodec::decodeDataReadyInfo(const void* data, size_t len, DataReadyInfo& info) {
    return false;
}

size_t MessageCodec::messageSize(const MessageHeader& hdr) {
    return 0;
}

bool MessageCodec::validateHeader(const MessageHeader& hdr) {
    return false;
}

MessageParser::MessageParser(Buffer& buf) : buffer_(buf), header_(nullptr) {
}

const MessageHeader* MessageParser::header() const {
    return header_;
}

const void* MessageParser::headerData() const {
    return nullptr;
}

const void* MessageParser::payload() const {
    return nullptr;
}

size_t MessageParser::payloadSize() const {
    return 0;
}

bool MessageParser::validate() const {
    return false;
}

bool MessageParser::hasPayload() const {
    return false;
}

bool MessageParser::isPullMode() const {
    return false;
}

const DataReadyInfo* MessageParser::dataReadyInfo() const {
    return nullptr;
}

}
