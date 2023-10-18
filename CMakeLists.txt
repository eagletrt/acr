cmake_minimum_required(VERSION 3.22)

project(ACR)

set(CMAKE_CXX_STANDARD 17)

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/inc
)

add_executable(main src/main.c)
