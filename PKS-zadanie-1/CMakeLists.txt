cmake_minimum_required(VERSION 3.9)
project(PKS_zadanie_1 C)

set(CMAKE_C_STANDARD 99)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

add_executable(PKS_zadanie_1 main.c header.h message_management.c sending.c recieving.c keep_alive.c global_variables.c)

target_link_libraries(PKS_zadanie_1 ws2_32.lib)