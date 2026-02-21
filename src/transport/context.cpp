#include <drpc/transport.h>

namespace drpc {

RdmaContext::RdmaContext(int device_index) {
}

RdmaContext::~RdmaContext() {
}

RdmaContext::RdmaContext(RdmaContext&& other) noexcept {
}

RdmaContext& RdmaContext::operator=(RdmaContext&& other) noexcept {
    return *this;
}

const char* RdmaContext::deviceName() const {
    return "";
}

int RdmaContext::deviceCount() {
    return 0;
}

std::vector<std::string> RdmaContext::listDevices() {
    return {};
}

}
