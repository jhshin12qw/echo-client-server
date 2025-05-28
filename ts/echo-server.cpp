#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <algorithm>

constexpr size_t BUFFER_SIZE = 1024;
constexpr int MAX_CLIENTS = 100;

bool flag_echo = false;
bool flag_broadcast = false;
std::vector<int> clients;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void* handle_client(void* arg) {
    int client_sock = *static_cast<int*>(arg);
    delete static_cast<int*>(arg);
    char buf[BUFFER_SIZE];
    
    while (true) {
        ssize_t n = recv(client_sock, buf, BUFFER_SIZE - 1, 0);
        if (n <= 0) break;
        buf[n] = '\0';
        std::cout << "Received: " << buf;

        if (flag_echo) {
            if (send(client_sock, buf, n, 0) < 0)
                perror("send");
        }
        if (flag_broadcast) {
            pthread_mutex_lock(&clients_mutex);
            for (int s : clients) {
                if (s != client_sock) {
                    if (send(s, buf, n, 0) < 0)
                        perror("broadcast send");
                }
            }
            pthread_mutex_unlock(&clients_mutex);
        }
    }
    
    pthread_mutex_lock(&clients_mutex);
    clients.erase(std::remove(clients.begin(), clients.end(), client_sock), clients.end());
    pthread_mutex_unlock(&clients_mutex);
    close(client_sock);
    return nullptr;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "syntax: echo-server <port> [-e][-b]" << std::endl;
        return 1;
    }
    int port = std::stoi(argv[1]);
    for (int i = 2; i < argc; ++i) {
        if (std::strcmp(argv[i], "-e") == 0) flag_echo = true;
        if (std::strcmp(argv[i], "-b") == 0) flag_broadcast = true;
    }

    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_sock < 0) { perror("socket"); return 1; }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(serv_sock, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr)) < 0) {
        perror("bind"); return 1;
    }
    if (listen(serv_sock, 5) < 0) { perror("listen"); return 1; }
    std::cout << "Server listening on port " << port << std::endl;

    while (true) {
        sockaddr_in cli_addr{};
        socklen_t cli_len = sizeof(cli_addr);
        int client_sock = accept(serv_sock, reinterpret_cast<sockaddr*>(&cli_addr), &cli_len);
        if (client_sock < 0) { perror("accept"); continue; }

        pthread_mutex_lock(&clients_mutex);
        if (clients.size() < MAX_CLIENTS) {
            clients.push_back(client_sock);
        } else {
            close(client_sock);
            pthread_mutex_unlock(&clients_mutex);
            continue;
        }
        pthread_mutex_unlock(&clients_mutex);

        int* pclient = new int(client_sock);
        pthread_t tid;
        pthread_create(&tid, nullptr, handle_client, pclient);
        pthread_detach(tid);
    }

    close(serv_sock);
    return 0;
}
