#include <drpc/transport.h>

namespace drpc {

CompletionQueue::CompletionQueue(RdmaContext& ctx, int cqe) {
}

CompletionQueue::~CompletionQueue() {
}

CompletionQueue::CompletionQueue(CompletionQueue&& other) noexcept {
}

CompletionQueue& CompletionQueue::operator=(CompletionQueue&& other) noexcept {
    return *this;
}

int CompletionQueue::poll(std::vector<ibv_wc>& wcs) {
    return 0;
}

int CompletionQueue::pollOne(ibv_wc& wc) {
    return 0;
}

}
