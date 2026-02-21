#include <drpc/dispatcher.h>

namespace drpc {

Dispatcher::Dispatcher(CompletionQueue& cq) : cq_(cq) {
}

Dispatcher::~Dispatcher() {
    stop();
}

void Dispatcher::start() {
}

void Dispatcher::stop() {
}

void Dispatcher::registerCallback(uint64_t wr_id, WCCallback cb) {
}

void Dispatcher::unregisterCallback(uint64_t wr_id) {
}

bool Dispatcher::hasCallback(uint64_t wr_id) const {
    return false;
}

void Dispatcher::pollOnce() {
}

void Dispatcher::pollLoop() {
}

}
