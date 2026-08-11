#include "Server.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <stdexcept>

namespace {
constexpr size_t kRecvBufferSize = 4096;

const char* kResponseBody = "Hello from cpp-multithread-server\n";
}  // namespace

Server::Server(uint16_t port, size_t numWorkers, int backlog)
    : port_(port), backlog_(backlog), serverFd_(-1), running_(false), pool_(numWorkers) {
    serverFd_ = createAndBindSocket();
}

Server::~Server() {
    stop();
    if (serverFd_ >= 0) {
        close(serverFd_);
    }
}

int Server::createAndBindSocket() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        throw std::runtime_error("socket() failed");
    }

    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(fd);
        throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(fd);
        throw std::runtime_error("bind() failed");
    }

    return fd;
}

void Server::run() {
    if (listen(serverFd_, backlog_) < 0) {
        throw std::runtime_error("listen() failed");
    }

    running_ = true;
    std::cout << "Server listening on port " << port_ << std::endl;
    acceptLoop();
}

void Server::acceptLoop() {
    while (running_) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);

        int clientFd = accept(serverFd_, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientFd < 0) {
            if (!running_) break;  // stop()으로 인한 정상 종료
            std::cerr << "accept() failed, errno=" << errno << std::endl;
            continue;
        }

        pool_.enqueue([this, clientFd] { handleClient(clientFd); });
    }
}

void Server::handleClient(int clientFd) {
    char buffer[kRecvBufferSize];
    ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0) {
        close(clientFd);
        return;
    }
    buffer[bytesRead] = '\0';

    // TODO: 실제 HTTP 파싱/라우팅은 여기서 buffer(요청 원문)를 기반으로 구현
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: " + std::to_string(std::strlen(kResponseBody)) + "\r\n"
        "Connection: close\r\n"
        "\r\n" +
        kResponseBody;

    send(clientFd, response.c_str(), response.size(), 0);
    close(clientFd);
}

void Server::stop() {
    if (!running_) return;
    running_ = false;

    // accept()가 블로킹 중이면 소켓을 닫아 깨워준다.
    if (serverFd_ >= 0) {
        shutdown(serverFd_, SHUT_RDWR);
    }
    pool_.shutdown();
}
