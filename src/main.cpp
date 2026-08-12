#include <csignal>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "Server.hpp"

namespace {
Server* g_server = nullptr; //서버 강제 종료시 운영체제가 접근해서 사용하는 용도

//종료 신호가 오면 server.stop() 호출
void handleSignal(int) {
    if (g_server) {
        g_server->stop();
    }
}
}  

int main() {
    const uint16_t port = 8080;
    const size_t numWorkers = std::thread::hardware_concurrency();  
                              //현재 컴퓨터의 CPU 코어(스레드) 개수를 알아내는 함수
                              //듀얼코어면 2, 헥사코어면 6 반환
    try {
        Server server(port, numWorkers > 0 ? numWorkers : 4);
        g_server = &server;

        std::signal(SIGINT, handleSignal);  //Ctrl+C 누르면 handleSignal 먼저 실행
        std::signal(SIGTERM, handleSignal); //kill 명령어로 종료시킬 때 handleSignal 먼저 실행

        server.run();  // accept 루프, stop() 호출 전까지 블로킹
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Server stopped." << std::endl;
    return EXIT_SUCCESS;
}
