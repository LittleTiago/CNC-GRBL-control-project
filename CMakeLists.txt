cmake_minimum_required(VERSION 3.16)
project(test_cnc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Busca dependências do Qt5 no Ubuntu
find_package(Qt5 REQUIRED COMPONENTS Core SerialPort)

# Inclui a pasta de serviços para que o compilador ache o CncService.h
include_directories(src/services)

# --- EXECUTÁVEL PRINCIPAL ---
add_executable(test_cnc
    src/main.cpp
    src/services/CncService.cpp
)
target_link_libraries(test_cnc PRIVATE Qt5::Core Qt5::SerialPort)

# --- EXECUTÁVEL DE DESENVOLVIMENTO (JOYSTICK) ---
# Adicionamos o main_dev.cpp aqui
add_executable(cnc_dev
    src/main_dev.cpp
    src/services/CncService.cpp
)
target_link_libraries(cnc_dev PRIVATE Qt5::Core Qt5::SerialPort)