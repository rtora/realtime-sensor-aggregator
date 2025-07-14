#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <thread>
#include <chrono>
#include "protocol.h" // Include our new header

const int PORT = 8080;

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    std::cout << "Aggregator attempting to connect..." << std::endl;
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        close(sock);
        return 1;
    }
    std::cout << "Connection established! Sending structured data..." << std::endl;

    SensorData data; // Create an instance of our data struct

    // --- Main loop to send dynamic data ---
    while (true) {
        // Update the data to simulate a moving sensor
        data.latitude += 0.0001;
        data.longitude -= 0.0001;
        data.temperature += 0.05;
        
        // Serialize the data struct into a string
        std::string serialized_data = data.serialize();
        
        // Send the serialized string
        ssize_t bytes_sent = send(sock, serialized_data.c_str(), serialized_data.size(), 0);
        
        if (bytes_sent <= 0) {
            std::cerr << "Failed to send data or connection closed." << std::endl;
            break;
        }

        std::cout << "Sent -> " << serialized_data << std::endl;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Send data twice a second
    }

    close(sock);
    return 0;
}