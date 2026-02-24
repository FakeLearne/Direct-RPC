#include <iostream>
#include <cstring>
#include <drpc/rpc.h>

int main() {
    drpc::RpcServer server("0.0.0.0", 12345);
    
    server.registerHandler(1, [](const void* req, size_t req_len, void* resp, size_t& resp_len) {
        std::cout << "Received: " << std::string((const char*)req, req_len) << std::endl;
        
        const char* reply = "Hello from server!";
        size_t len = strlen(reply);
        memcpy(resp, reply, len);
        resp_len = len;
    });
    
    if (!drpc::is_ok(server.start())) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    
    std::cout << "Server started on port 12345" << std::endl;
    server.run();
    
    return 0;
}
