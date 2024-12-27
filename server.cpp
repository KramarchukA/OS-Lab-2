#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <csignal>
#include <vector>
#include <algorithm>
#define PORT 8080
#define BUFFER_SIZE 1024

volatile sig_atomic_t wasSigHup = 0;

// Обработчик сигнала SIGHUP
void sigHupHandler(int) {
    wasSigHup = 1;
}

int main() {
    int serverSocket;
    struct sockaddr_in serverAddr;
    std::vector<int> clientSockets;
    char buffer[BUFFER_SIZE];

    // Регистрация обработчика сигнала
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigHupHandler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGHUP, &sa, nullptr);

    // Блокировка сигнала SIGHUP
    sigset_t blockedMask, origMask;
    sigemptyset(&blockedMask);
    sigaddset(&blockedMask, SIGHUP);
    sigprocmask(SIG_BLOCK, &blockedMask, &origMask);

    // Создание серверного сокета
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("Socket creation error");
        return 1;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Socket binding error");
        close(serverSocket);
        return 1;
    }

    if (listen(serverSocket, 5) == -1) {
        perror("Listening error");
        close(serverSocket);
        return 1;
    }
    std::cout << "Server is listening on port "<< PORT << "..." << std::endl;

    // Основной цикл
    while (true) {
        fd_set fds;
        FD_ZERO(&fds);

        FD_SET(serverSocket, &fds);
        int maxFd = serverSocket;

        for (int clientSocket : clientSockets) {
            FD_SET(clientSocket, &fds);
            maxFd = std::max(maxFd, clientSocket);
        }

        int activity = pselect(maxFd + 1, &fds, nullptr, nullptr, nullptr, &origMask);
        if (activity == -1) {
            if (errno == EINTR) {
                if (wasSigHup) {
                    std::cout << "Received SIGHUP signal" << std::endl;
                    wasSigHup = 0;
                }
                continue;
            } else {
                perror("pselect error");
                break;
            }
        }

        if (FD_ISSET(serverSocket, &fds)) {
            struct sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);
            int newClientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
            if (newClientSocket < 0) {
                perror("Error accepting client connection");
                continue;
            }

            clientSockets.push_back(newClientSocket);
            std::cout << "New client connected!" << std::endl;
        }

        // Обработка активности на клиентских сокетах
        for (auto it = clientSockets.begin(); it != clientSockets.end(); it++) {
            int clientSocket = *it;
            if (FD_ISSET(clientSocket, &fds)) {
                memset(buffer, 0, BUFFER_SIZE);
                ssize_t bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
                if (bytesRead <= 0) {
                    if (bytesRead == 0) {
                        std::cout << "Client disconnected" << std::endl;
                    } else {
                        perror("Error reading data from client");
                    }
                    close(clientSocket);
                    it = clientSockets.erase(it);
                    continue;
                } else {
                    std::cout << "Message from client: "<< buffer << std::endl;
                }
            }
        }
    }

    for (int clientSocket : clientSockets) {
        close(clientSocket);
    }

    close(serverSocket);
    return 0;
}
