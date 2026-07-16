#include "network/EpollServer.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

EpollServer::EpollServer(int port) : port(port), running(false) {
    // 1. Create the server socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "[ERROR] Failed to create socket\n";
        exit(EXIT_FAILURE);
    }

    // Allow port reuse so we don't get "Address already in use" during rapid testing
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 2. Bind the socket
    struct sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "[ERROR] Bind failed on port " << port << "\n";
        exit(EXIT_FAILURE);
    }

    // 3. Make server socket non-blocking
    set_non_blocking(server_fd);

    // 4. Listen for connections
    if (listen(server_fd, SOMAXCONN) < 0) {
        std::cerr << "[ERROR] Listen failed\n";
        exit(EXIT_FAILURE);
    }

    // 5. Create epoll instance
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::cerr << "[ERROR] Failed to create epoll instance\n";
        exit(EXIT_FAILURE);
    }

    // 6. Add server socket to epoll to monitor for incoming connections (EPOLLIN)
    struct epoll_event event{};
    event.data.fd = server_fd;
    event.events = EPOLLIN | EPOLLET; // Edge-triggered
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);
}

EpollServer::~EpollServer() {
    close(server_fd);
    close(epoll_fd);
}

void EpollServer::set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void EpollServer::start() {
    running = true;
    std::cout << "[INFO] Vortex Server Event Loop started on port " << port << "...\n";

    while (running) {
        // Wait indefinitely for an event
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        
        for (int i = 0; i < num_events; i++) {
            if (events[i].data.fd == server_fd) {
                // We have a new connection
                accept_connection();
            } else {
                // We have data from an existing client
                handle_client_data(events[i].data.fd);
            }
        }
    }
}

void EpollServer::stop() {
    running = false;
}

void EpollServer::accept_connection() {
    struct sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    
    // Accept in a loop because multiple clients might have connected at once
    while (true) {
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // We have processed all incoming connections
            }
            std::cerr << "[ERROR] Accept failed\n";
            break;
        }

        std::cout << "[NETWORK] New connection established! (FD: " << client_fd << ")\n";
        
        // Make the new client non-blocking
        set_non_blocking(client_fd);

        // Add the client to epoll
        struct epoll_event event{};
        event.data.fd = client_fd;
        event.events = EPOLLIN | EPOLLET;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
    }
}

void EpollServer::handle_client_data(int client_fd) {
    char buffer[1024];
    
    // Read in a loop until the kernel buffer is empty (required for EPOLLET)
    while (true) {
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            std::cout << "[NETWORK] Received " << bytes_read << " bytes from FD " << client_fd << ": " << buffer;
        } else if (bytes_read == 0) {
            std::cout << "[NETWORK] Client (FD: " << client_fd << ") disconnected.\n";
            close(client_fd); // Removing a closed FD from epoll is automatic in newer Linux kernels
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // We have read all available data for now
                break;
            }
            std::cerr << "[ERROR] Read error on FD " << client_fd << "\n";
            close(client_fd);
            break;
        }
    }
}