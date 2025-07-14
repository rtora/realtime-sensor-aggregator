#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <string>
#include "protocol.h" // Include the shared protocol header

const int PORT = 8080;

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to bind socket" << std::endl;
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 3) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(server_fd);
        return 1;
    }

    std::cout << "Listener is waiting for a connection on port " << PORT << "..." << std::endl;
    
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

    if (client_fd < 0) {
        std::cerr << "Failed to accept connection" << std::endl;
        close(server_fd);
        return 1;
    }
    std::cout << "Connection accepted! Now receiving structured data..." << std::endl;

    // --- Main loop to receive data ---
    std::vector<char> buffer(1024);
    while (true) {
        buffer.assign(buffer.size(), 0);
        ssize_t bytes_received = recv(client_fd, buffer.data(), buffer.size(), 0);

        if (bytes_received <= 0) {
            // Client disconnected or error occurred
            std::cout << "Client disconnected." << std::endl;
            break;
        }

        std::string received_str(buffer.begin(), buffer.begin() + bytes_received);
        // For now, just print the raw data. Later we'll parse it.
        std::cout << "Received <- " << received_str << std::endl;
    }

    close(client_fd);
    close(server_fd);
    return 0;
}