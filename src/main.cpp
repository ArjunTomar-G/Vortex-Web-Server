#include "network/EpollServer.hpp"
#include <iostream>

int main() {
    std::cout << "==========================================\n";
    std::cout << "       VORTEX ENGINE: INITIALIZING        \n";
    std::cout << "==========================================\n";

    // Boot the server on port 8080
    EpollServer server(8080);
    
    // Start the infinite I/O loop
    server.start();

    return 0;
}