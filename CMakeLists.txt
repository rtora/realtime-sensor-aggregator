cmake_minimum_required(VERSION 3.10)
project(RealtimeSensorAggregator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

set(IMGUI_SOURCES
    imgui.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    imgui_impl_sdl2.cpp
    imgui_impl_opengl3.cpp
)

add_executable(listener listener.cpp ${IMGUI_SOURCES})

target_include_directories(listener PRIVATE ${SDL2_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR})
target_link_libraries(listener PRIVATE ${SDL2_LIBRARIES} ${OPENGL_gl_LIBRARY})

# Add this line to silence the OpenGL deprecation warnings on macOS
target_compile_definitions(listener PRIVATE GL_SILENCE_DEPRECATION)

add_executable(aggregator aggregator.cpp)