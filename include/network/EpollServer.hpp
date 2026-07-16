#pragma once

#include <sys/epoll.h>
#include <memory>
#include "concurrency/ThreadPool.hpp"

#define MAX_EVENTS 64

class EpollServer {
private:
    int server_fd;
    int epoll_fd;
    int port;
    bool running;
    struct epoll_event events[MAX_EVENTS];

    std::unique_ptr<ThreadPool> thread_pool;

    void set_non_blocking(int fd);
    void accept_connection();
    void handle_client_data(int client_fd);

public:
    explicit EpollServer(int port);
    ~EpollServer();
    
    void start();
    void stop();
};