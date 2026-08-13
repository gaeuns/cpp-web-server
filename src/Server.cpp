#include "Server.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <fstream>
#include <string>

using namespace std;

namespace {
constexpr size_t kRecvBufferSize = 4096;

const char* kResponseBody = "Hello from cpp-multithread-server\n";
}  // namespace

Server::Server(uint16_t port, size_t numWorkers, int backlog)
    : port_(port), backlog_(backlog), serverFd_(-1), running_(false), pool_(numWorkers) {
    serverFd_ = createAndBindSocket();  // 서버 객체가 만들어질 때 통신용 소켓을 바로 생성
}

Server::~Server() {
    stop();
    if (serverFd_ >= 0) {
        close(serverFd_);
    }
}

int Server::createAndBindSocket() {
                    // IPv4 주소체계, TCP 방식 사용
    int fd = socket(AF_INET, SOCK_STREAM, 0);   // 만들어진 소켓은 파일 디스크립터라는 정수로 OS가 관리
    if (fd < 0) {
        throw runtime_error("socket() failed");
    }

    //포트 묶임 방지
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(fd);
        throw runtime_error("setsockopt(SO_REUSEADDR) failed");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;  // 내 컴퓨터의 모든 IP에서 접속 허용
    addr.sin_port = htons(port_);   // 네트워크 표준에 맞게 바이트 순서를 뒤집음 

    // 소켓을 8080 포트에 붙임
    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(fd);
        throw runtime_error("bind() failed");
    }

    return fd;
}

void Server::run() {
    if (listen(serverFd_, backlog_) < 0) {  // 대기열 생성
        throw runtime_error("listen() failed");
    }

    running_ = true;
    cout << "Server listening on port " << port_ << endl;
    acceptLoop();
}

// 클라이언트가 접속할 때까지 여기서 무한 대기
// 접속 성공 시 해당 클라이언트에 새로운 소켓 번호(clientFd) 발급
void Server::acceptLoop() {
    while (running_) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);

        // accept()로 클라이언트 연결을 하나씩 받음 
        int clientFd = accept(serverFd_, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientFd < 0) {
            if (!running_) break;  // stop()으로 인한 정상 종료
            cerr << "accept() failed, errno=" << errno << endl;
            continue;
        }

        // 처리를 직접 하지 않고 스레드풀에 작업으로 던짐(큐에 쌓임)
        pool_.enqueue([this, clientFd] { handleClient(clientFd); });
    }
}

// 실제로 요청을 읽고(recv) 응답을 보내는(send) 부분
void Server::handleClient(int clientFd) {
    char buffer[kRecvBufferSize];
    ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

    if (bytesRead <= 0) {
        close(clientFd);
        return;
    }
    buffer[bytesRead] = '\0';

    // buffer(요청 원문)를 한 줄씩 파싱할 수 있도록 istringstream으로 감쌈
    istringstream requestStream(buffer);
    string methode, path, version;

    requestStream >> methode >>path >> version;

    cout << "methode : " << methode << "\n";
    cout << "path : " << path << "\n";
    cout << "version : " << version << "\n";
    cout << "--------------------------\n";

    //cout << "--- 클라이언트 요청 원문 ---\n";
    //cout << buffer << "\n";
    // cout << "--------------------------\n";

    string status, body;
    string realPath = path;
    string query;
    string filePath = "../public/index.html";
    string fileType = "text/html";
    size_t qMark = path.find("?");

    // 쿼리 분리, 파일 경로 지정
    if(qMark!=string::npos){
        realPath = path.substr(0, qMark);
        query = path.substr(qMark + 1);
        filePath = "../public" + realPath;
    }
    else if(path!="/"){
        filePath = "../public" + realPath;
    }

    if(path.find(".css") != string::npos){
        fileType = "text/css";
    }
    if (realPath.find("/api/") == 0) {
        fileType = "application/json; charset=utf-8";
        status = "200 OK";

        if (realPath == "/api/search") {
            body = R"({
                "status": "success",
                "query": ")" + query + R"(",
                "message": "데이터를 성공적으로 찾았습니다."
            })";
        }
        else if (realPath == "/api/hello") {
            body = R"({"status": "success", "message": "하이"})";
        }
        else {
            status = "404 Not Found";
            body = R"({"status": "error", "message": "API 경로를 찾을 수 없습니다."})";
        }
    }
    else{
        ifstream file(filePath);
        if(file.is_open()){
            ostringstream ost;
            ost << file.rdbuf();
            body = ost.str();
            status = "200 OK";
        }
        else{
            status = "404 Not Found";
            body = "404 Page Not Found";
        }
    }

    // 클라이언트에게 보낼 HTTP 응답 규격을 문자열로 만들어서 전송하고 연결 해제
    string response =
        version + " " + status + "\r\n"   // HTTP 규격상 줄바꿈은 \r\n 사용
        "Content-Type: " + fileType + "\r\n"
        "Content-Length: " + to_string(body.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" +    // 헤더와 바디 사이에 빈 줄이 하나 있어야함
        body;

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
