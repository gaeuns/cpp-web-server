#include <csignal>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "Server.hpp"

namespace {
Server* g_server = nullptr;

void handleSignal(int) {
    if (g_server) {
        g_server->stop();
    }
}
}  // namespace

int main() {
    const uint16_t port = 8080;
    const size_t numWorkers = std::thread::hardware_concurrency();

    try {
        Server server(port, numWorkers > 0 ? numWorkers : 4);
        g_server = &server;

        std::signal(SIGINT, handleSignal);
        std::signal(SIGTERM, handleSignal);

        server.run();  // accept 루프, stop() 호출 전까지 블로킹
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Server stopped." << std::endl;
    return EXIT_SUCCESS;
}
