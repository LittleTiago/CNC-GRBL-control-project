cmake_minimum_required(VERSION 3.16)
project(test_cnc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS Core SerialPort)

add_executable(test_cnc
    src/main.cpp
    src/services/CncService.h
    src/services/CncService.cpp
)

# --- ADICIONE ESTA LINHA ABAIXO ---
target_include_directories(test_cnc PRIVATE src/services)
# ----------------------------------

target_link_libraries(test_cnc PRIVATE Qt5::Core Qt5::SerialPort)