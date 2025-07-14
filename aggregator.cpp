#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <thread>
#include <chrono>
#include <fcntl.h>
#include "protocol.h"

const int PORT = 8080;

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { /* ... */ return 1; }

    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    
    std::cout << "Aggregator attempting to connect..." << std::endl;
    while (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        if (errno == EINPROGRESS || errno == EALREADY) {} 
        else if (errno == EISCONN) { break; } 
        else { std::cerr << "Connection Failed" << std::endl; close(sock); return 1; }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "Connection established! Sending data and listening for commands..." << std::endl;
    
    SensorData data;
    const SensorData initial_data = data; // Store initial state for boundaries
    double lat_velocity = 0.0;
    double lon_velocity = 0.0;
    char buffer[1024] = {0};

    // --- New: Define boundaries for the simulation ---
    const double max_offset = 0.005; 

    while (true) {
        ssize_t bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string command(buffer);
            std::cout << "Received command: " << command << std::endl;

            if (command == "CMD:N") { lat_velocity = 0.0001; lon_velocity = 0.0; }
            else if (command == "CMD:S") { lat_velocity = -0.0001; lon_velocity = 0.0; }
            else if (command == "CMD:E") { lat_velocity = 0.0; lon_velocity = 0.0001; }
            else if (command == "CMD:W") { lat_velocity = 0.0; lon_velocity = -0.0001; }
        }

        data.latitude += lat_velocity;
        data.longitude += lon_velocity;
        
        // --- New: Boundary checking logic ---
        if (data.latitude >= initial_data.latitude + max_offset) { data.latitude = initial_data.latitude + max_offset; lat_velocity = 0; }
        if (data.latitude <= initial_data.latitude - max_offset) { data.latitude = initial_data.latitude - max_offset; lat_velocity = 0; }
        if (data.longitude >= initial_data.longitude + max_offset) { data.longitude = initial_data.longitude + max_offset; lon_velocity = 0; }
        if (data.longitude <= initial_data.longitude - max_offset) { data.longitude = initial_data.longitude - max_offset; lon_velocity = 0; }

        data.temperature += 0.01;
        if (data.temperature > 100.0) data.temperature = 100.0;
        
        std::string serialized_data = data.serialize();
        send(sock, serialized_data.c_str(), serialized_data.size(), 0);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    close(sock);
    return 0;
}