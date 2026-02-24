#include <iostream>
#include <cstring>
#include <drpc/rpc.h>

int main() {
    drpc::RpcClient client;
    
    if (!drpc::is_ok(client.connect("192.168.100.2", 12345))) {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }
    
    std::cout << "Connected to server" << std::endl;
    
    const char* msg = "Hello from client!";
    char resp[1024];
    size_t resp_len = sizeof(resp);
    
    if (drpc::is_ok(client.call(1, msg, strlen(msg), resp, resp_len))) {
        std::cout << "Response: " << std::string(resp, resp_len) << std::endl;
    } else {
        std::cerr << "RPC call failed" << std::endl;
    }
    
    client.disconnect();
    return 0;
}
