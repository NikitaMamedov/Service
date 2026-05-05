cmake_minimum_required(VERSION 3.20)
project(TrayApp LANGUAGES CXX)

add_executable(TrayApp WIN32 src/main.cpp)

target_link_libraries(TrayApp PRIVATE user32 shell32)