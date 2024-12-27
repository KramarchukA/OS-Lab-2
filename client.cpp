#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>


class Client {
public:
    Client(const std::string& server_ip, int port) {
        // Создание сокета
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            std::cerr << "Socket creation error" << std::endl;
            exit(1);
        }

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = inet_addr(server_ip.c_str());
    }

    void connectToServer() {
        if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Server connection error" << std::endl;
            close(sock);
            exit(1);
        }

        std::cout << "Connected to the server" << std::endl;
    }

    void sendMessage(const std::string& message) {
        if (send(sock, message.c_str(), message.size(), 0) < 0) {
            std::cerr << "Data sending error" << std::endl;
        } else {
            std::cout << "Message sent: " << message << std::endl;
        }
    }

    ~Client() {
        close(sock);
    }

private:
    int sock;
    struct sockaddr_in serverAddr;
};


void runClient(const std::string& server_ip, int port) {
    Client client(server_ip, port);
    client.connectToServer();
    int i = 0;
    while (true){
        std::string message = "This is my message №" + std::to_string(i++);
        client.sendMessage(message);
    }
}

int main() {
    std::string server_ip = "127.0.0.1";  
    int port = 8080;  

    runClient (server_ip, port);

    return 0;
}
