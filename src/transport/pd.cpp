#include <drpc/transport.h>

namespace drpc {

ProtectionDomain::ProtectionDomain(RdmaContext& ctx) {
}

ProtectionDomain::~ProtectionDomain() {
}

ProtectionDomain::ProtectionDomain(ProtectionDomain&& other) noexcept {
}

ProtectionDomain& ProtectionDomain::operator=(ProtectionDomain&& other) noexcept {
    return *this;
}

}
