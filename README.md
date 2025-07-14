# Real-Time USV Telemetry Simulator

This project is a C++ application that simulates a command and control (C2) system for an Unmanned Surface Vessel (USV). It features a real-time, interactive GUI that displays telemetry data and allows the user to send movement commands to the simulated USV.

This project was developed as a demonstration of skills in C++ systems programming, networking, multithreading, and GUI development, inspired by the challenges in modern defense technology.

### Demo

*(It is highly recommended to create a short screen recording or GIF of the final application and place it here. For now, you can use a screenshot.)*

![Demo Screenshot](demo_screenshot.png)

---

## Core Features

* **Client-Server Architecture:** A networked application with a `listener` (the C2 GUI) and an `aggregator` (the USV simulation).
* **Real-Time GUI:** A dashboard built with Dear ImGui and SDL2 displays live telemetry from the USV.
* **Interactive C2:** The GUI can send movement commands back to the USV simulation to control it in real-time.
* **Keyboard and UI Controls:** The USV can be controlled with both on-screen buttons and the keyboard arrow keys.
* **Tactical Map Display:** A simple 2D map visualizes the USV's position, heading, and recent trail.
* **Multithreaded Design:** The GUI and networking operations run on separate threads to ensure a responsive, non-blocking user interface.

## Technical Concepts Demonstrated

This project is a practical demonstration of several key software engineering concepts:

* **Languages:** C++17
* **Build System:** CMake
* **Networking:** Low-level TCP Sockets (POSIX standard)
* **Concurrency:** `std::thread` for multithreading and `std::mutex` for thread-safe data sharing.
* **Graphics & GUI:** Dear ImGui with an SDL2 & OpenGL backend.
* **Software Architecture:** Client-server design, data serialization/deserialization, and event-driven programming.

---

## Building and Running on macOS

### Prerequisites

* Xcode Command Line Tools (`xcode-select --install`)
* Homebrew
* CMake: `brew install cmake`
* SDL2: `brew install sdl2`

### Running the Application

1.  **Clone the Repository:**
    ```bash
    git clone [https://github.com/YourUsername/realtime-sensor-aggregator.git](https://github.com/YourUsername/realtime-sensor-aggregator.git)
    cd realtime-sensor-aggregator
    ```

2.  **Build the Project:**
    ```bash
    # Create a build directory
    mkdir build
    cd build

    # Configure and build using CMake
    cmake ..
    make
    ```

3.  **Run the Demo:**
    You will need two separate terminal windows open in the `build` directory.

    * In **Terminal 1**, run the listener GUI:
        ```bash
        ./listener
        ```
    * In **Terminal 2**, run the USV simulation:
        ```bash
        ./aggregator
        ```
    * The GUI will appear, and once the aggregator connects, you can control the USV with the arrow keys or on-screen buttons.

---

## Code Highlights

Here are a few snippets that showcase key parts of the project's design.

### 1. Multithreaded Networking

To keep the GUI responsive, all blocking network operations are handled on a separate thread. The main thread is free to render the UI at 60 FPS while the network thread waits for connections and data.

**`listener.cpp`**:
```cpp
// Global mutex to protect shared data between threads
std::mutex g_data_mutex;
SensorData g_sensor_data;

// This function runs on its own thread, handling all network I/O
void network_thread_func() {
    // ... socket, bind, listen logic ...

    std::cout << "Net: Waiting for connection...\n";
    g_client_fd = accept(server_fd, nullptr, nullptr);
    // ...

    // Loop to continuously receive data
    while (g_client_fd != -1) {
        ssize_t bytes = recv(g_client_fd, buffer.data(), buffer.size() - 1, 0);
        if (bytes <= 0) {
            std::cout << "Net: Client disconnected.\n";
            break;
        }

        // When data arrives, lock the mutex to safely update the shared data
        std::lock_guard<std::mutex> lock(g_data_mutex);
        g_sensor_data = SensorData::deserialize(received_str);
    }
}

int main() {
    // Launch the network thread at the start of the application
    std::thread network_thread(network_thread_func);

    // The main thread continues here, dedicated to running the GUI loop
    while (!done) {
        // ... GUI rendering logic ...
    }

    // Wait for the network thread to finish before exiting
    network_thread.join();
    return 0;
}
```

### 2. Two-Way Communication & Non-Blocking Sockets

The `aggregator` (USV) needs to both send telemetry and listen for commands from the C2 GUI. This is achieved by setting its socket to non-blocking mode, allowing it to check for incoming data without getting "stuck."

**`aggregator.cpp`**:
```cpp
// Set the client socket to non-blocking mode
fcntl(sock, F_SETFL, O_NONBLOCK);

// The main simulation loop
while (true) {
    // 1. Check for commands from the listener (non-blocking)
    ssize_t bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        // A command was received, update vehicle velocity
        std::string command(buffer);
        if (command == "CMD:N") { /* ... set velocity north ... */ }
        // ...
    }

    // 2. Update the USV's position based on current velocity
    data.latitude += lat_velocity;
    data.longitude += lon_velocity;

    // 3. Send the updated telemetry back to the listener
    std::string serialized_data = data.serialize();
    send(sock, serialized_data.c_str(), serialized_data.size(), 0);

    // 4. Sleep for a short duration to run the simulation at a fixed rate
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
```