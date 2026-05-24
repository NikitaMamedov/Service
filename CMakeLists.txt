cmake_minimum_required(VERSION 3.20)
project(TrayService LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 17)

# ================== SERVICE ==================
add_executable(TrayService
    src/service/main.cpp
    src/service/service.cpp
    src/rpc/rpc_server.cpp
    src/rpc/tray_s.c
)
target_link_libraries(TrayService PRIVATE advapi32 user32 wtsapi32 rpcrt4)
target_include_directories(TrayService PRIVATE src src/rpc)

# ================== TRAY APP ==================
add_executable(TrayApp WIN32
    src/app/main.cpp
    src/app/rpc_memory.c
    src/rpc/tray_c.c
)
target_link_libraries(TrayApp PRIVATE user32 shell32 gdi32 rpcrt4)
target_include_directories(TrayApp PRIVATE src src/rpc)