#pragma once

#include <atomic>
#include <cstdint>

#include "ThreadPool.hpp"

class Server {
public:
    Server(uint16_t port, size_t numWorkers, int backlog = 128);
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void run();
    void stop();

private:
    int createAndBindSocket();
    void acceptLoop();
    void handleClient(int clientFd);

    uint16_t port_;
    int backlog_;
    int serverFd_;
    std::atomic<bool> running_;
    ThreadPool pool_;
};
