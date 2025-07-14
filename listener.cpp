#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <deque>
#include "protocol.h"

// SDL / OpenGL / ImGui Headers
#include <SDL2/SDL.h>
#include <OpenGL/gl.h>
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

// Networking headers
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

std::mutex g_data_mutex;
SensorData g_sensor_data;
SensorData g_origin_data; // To fix the trail
bool g_origin_set = false;  // Flag for the origin
int g_client_fd = -1;
const int PORT = 8080;
std::deque<ImVec2> g_usv_trail;
const size_t MAX_TRAIL_LENGTH = 100;

void network_thread_func() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { std::cerr << "Net: Socket failed\n"; return; }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) { std::cerr << "Net: Bind failed\n"; close(server_fd); return; }
    if (listen(server_fd, 3) < 0) { std::cerr << "Net: Listen failed\n"; close(server_fd); return; }

    std::cout << "Net: Waiting for connection...\n";
    g_client_fd = accept(server_fd, nullptr, nullptr);
    if (g_client_fd < 0) { std::cerr << "Net: Accept failed\n"; close(server_fd); return; }
    std::cout << "Net: Connection accepted!\n";

    std::vector<char> buffer(1024);
    while (g_client_fd != -1) {
        ssize_t bytes = recv(g_client_fd, buffer.data(), buffer.size() - 1, 0);
        if (bytes <= 0) { std::cout << "Net: Client disconnected.\n"; break; }
        
        buffer[bytes] = '\0';
        std::string received_str(buffer.data());
        
        std::lock_guard<std::mutex> lock(g_data_mutex);
        g_sensor_data = SensorData::deserialize(received_str);
        if (!g_origin_set) {
            g_origin_data = g_sensor_data;
            g_origin_set = true;
        }
    }
    if (g_client_fd != -1) close(g_client_fd);
    g_client_fd = -1;
    close(server_fd);
}

int main() {
    std::thread network_thread(network_thread_func);

    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_Window* window = SDL_CreateWindow("Anduril Mission Control", 100, 100, 1000, 700, SDL_WINDOW_OPENGL);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 150");

    bool done = false;
    static char active_direction = 'N';

    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) done = true;
            
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP:    send(g_client_fd, "CMD:N", 5, 0); active_direction = 'N'; break;
                    case SDLK_DOWN:  send(g_client_fd, "CMD:S", 5, 0); active_direction = 'S'; break;
                    case SDLK_RIGHT: send(g_client_fd, "CMD:E", 5, 0); active_direction = 'E'; break;
                    case SDLK_LEFT:  send(g_client_fd, "CMD:W", 5, 0); active_direction = 'W'; break;
                }
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        
        SensorData current_data;
        {
            std::lock_guard<std::mutex> lock(g_data_mutex);
            current_data = g_sensor_data;
        }

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
        ImGui::Begin("Controls & Telemetry");
        
        ImGui::Text("USV Telemetry");
        ImGui::Separator();
        ImGui::Text("Latitude:  %.4f", current_data.latitude);
        ImGui::Text("Longitude: %.4f", current_data.longitude);

        // --- The Fix: Give the progress bar a specific size ---
        ImGui::ProgressBar(current_data.temperature / 100.0f, ImVec2(180.0f, 0.0f)); 

        ImGui::SameLine();
        ImGui::Text("%.1f C", current_data.temperature);
        
        ImGui::Separator();
        ImGui::Text("Movement Controls");

        // --- Fix: New robust logic for highlighting buttons ---
        if (active_direction == 'N') ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        if (ImGui::Button("North")) { send(g_client_fd, "CMD:N", 5, 0); active_direction = 'N'; }
        if (active_direction == 'N') ImGui::PopStyleColor(1);
        ImGui::SameLine();

        if (active_direction == 'S') ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        if (ImGui::Button("South")) { send(g_client_fd, "CMD:S", 5, 0); active_direction = 'S'; }
        if (active_direction == 'S') ImGui::PopStyleColor(1);
        ImGui::SameLine();

        if (active_direction == 'E') ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        if (ImGui::Button("East"))  { send(g_client_fd, "CMD:E", 5, 0); active_direction = 'E'; }
        if (active_direction == 'E') ImGui::PopStyleColor(1);
        ImGui::SameLine();

        if (active_direction == 'W') ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        if (ImGui::Button("West"))  { send(g_client_fd, "CMD:W", 5, 0); active_direction = 'W'; }
        if (active_direction == 'W') ImGui::PopStyleColor(1);

        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(320, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(670, 680), ImGuiCond_FirstUseEver);
        ImGui::Begin("Tactical Map");
        ImVec2 map_pos = ImGui::GetCursorScreenPos();
        ImVec2 map_size = ImGui::GetContentRegionAvail();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        draw_list->AddRect(map_pos, ImVec2(map_pos.x + map_size.x, map_pos.y + map_size.y), IM_COL32(255, 255, 0, 255));
        for (float i = 0; i <= map_size.x; i += 50) draw_list->AddLine(ImVec2(map_pos.x + i, map_pos.y), ImVec2(map_pos.x + i, map_pos.y + map_size.y), IM_COL32(255, 255, 0, 50));
        for (float i = 0; i <= map_size.y; i += 50) draw_list->AddLine(ImVec2(map_pos.x, map_pos.y + i), ImVec2(map_pos.x + map_size.x, map_pos.y + i), IM_COL32(255, 255, 0, 50));

        // --- Fix: Adjusted scaling factor for longitude to balance speed ---
        float lat_scale = 60000.0f; 
        float lon_scale = lat_scale * 0.75; 
        
        float point_x = map_pos.x + map_size.x / 2.0f + (current_data.longitude - g_origin_data.longitude) * lon_scale;
        float point_y = map_pos.y + map_size.y / 2.0f - (current_data.latitude - g_origin_data.latitude) * lat_scale;
        ImVec2 usv_pos(point_x, point_y);

        g_usv_trail.push_back(usv_pos);
        if (g_usv_trail.size() > MAX_TRAIL_LENGTH) g_usv_trail.pop_front();
        if (g_usv_trail.size() > 1) {
             for (size_t i = 0; i < g_usv_trail.size() - 1; ++i) {
                float alpha = (float)i / MAX_TRAIL_LENGTH;
                draw_list->AddLine(g_usv_trail[i], g_usv_trail[i+1], IM_COL32(0, 255, 0, alpha * 255), 2.0f);
            }
        }
        
        ImVec2 p1, p2, p3;
        float r = 6.0f;
        switch (active_direction) {
            case 'N': p1 = ImVec2(usv_pos.x, usv_pos.y - r); p2 = ImVec2(usv_pos.x - r*0.8f, usv_pos.y + r*0.8f); p3 = ImVec2(usv_pos.x + r*0.8f, usv_pos.y + r*0.8f); break;
            case 'S': p1 = ImVec2(usv_pos.x, usv_pos.y + r); p2 = ImVec2(usv_pos.x - r*0.8f, usv_pos.y - r*0.8f); p3 = ImVec2(usv_pos.x + r*0.8f, usv_pos.y - r*0.8f); break;
            case 'E': p1 = ImVec2(usv_pos.x + r, usv_pos.y); p2 = ImVec2(usv_pos.x - r*0.8f, usv_pos.y - r*0.8f); p3 = ImVec2(usv_pos.x - r*0.8f, usv_pos.y + r*0.8f); break;
            case 'W': p1 = ImVec2(usv_pos.x - r, usv_pos.y); p2 = ImVec2(usv_pos.x + r*0.8f, usv_pos.y - r*0.8f); p3 = ImVec2(usv_pos.x + r*0.8f, usv_pos.y + r*0.8f); break;
        }
        draw_list->AddTriangleFilled(p1, p2, p3, IM_COL32(0, 255, 0, 255));
        
        ImGui::End();

        ImGui::Render();
        glViewport(0, 0, 1000, 700);
        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
    
    // Shutdown sequence...
    if (g_client_fd != -1) {
        network_thread.detach(); 
        close(g_client_fd);
    } else {
        network_thread.join();
    }
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}