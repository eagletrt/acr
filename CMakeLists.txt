cmake_minimum_required(VERSION 3.18)

project(ACR)

set(CMAKE_CXX_STANDARD 17)

find_library(pigpio_LIBRARY 
	NAMES libpigpio.so
	HINTS /usr/local/lib)

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/inc
)

add_library(acr
	STATIC
	src/led.c
)
target_link_libraries(acr PUBLIC ${pigpio_LIBRARY})

add_executable(main src/main.c)
target_link_libraries(main acr)
