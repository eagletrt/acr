cmake_minimum_required(VERSION 3.18)

project(ACR)

set(CMAKE_CXX_STANDARD 17)

find_library(pigpio_LIBRARY 
	NAMES libpigpio.so
	HINTS /usr/local/lib
)
if(${pigpio_LIBRARY} STREQUAL "pigpio_LIBRARY-NOTFOUND")
	message(WARNING "pigpio not found")
	set(pigpio_LIBRARY "")
	add_compile_definitions(ACR_NO_PIGPIO)
endif()

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/inc
    ${CMAKE_CURRENT_LIST_DIR}/external
)

add_library(acr
	STATIC
	${CMAKE_CURRENT_LIST_DIR}/src/led.c
	${CMAKE_CURRENT_LIST_DIR}/src/gpio.c
	${CMAKE_CURRENT_LIST_DIR}/src/utils.c
)
target_link_libraries(acr PUBLIC ${pigpio_LIBRARY})

add_library(gps
	STATIC
	${CMAKE_CURRENT_LIST_DIR}/external/gpslib/gps.c
	${CMAKE_CURRENT_LIST_DIR}/external/gpslib/gps_interface.c
)

add_executable(main src/main.c)
target_link_libraries(main acr gps m)
