#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <algorithm>

constexpr size_t BUFFER_SIZE = 1024;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "syntax: echo-client <ip> <port>" << std::endl;
        return 1;
    }
    const std::string ip = argv[1];
    int port = std::stoi(argv[2]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "invalid address" << std::endl;
        return 1;
    }

    if (connect(sock, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr)) < 0) {
        perror("connect"); return 1;
    }

    char buf[BUFFER_SIZE];
    while (std::cin.getline(buf, BUFFER_SIZE)) {
        size_t len = std::strlen(buf);
        buf[len++] = '\n';
        if (send(sock, buf, len, 0) < 0) { perror("send"); break; }
        ssize_t n = recv(sock, buf, BUFFER_SIZE - 1, 0);
        if (n <= 0) break;
        buf[n] = '\0';
        std::cout << buf;
    }

    close(sock);
    return 0;
}
