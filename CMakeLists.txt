cmake_minimum_required(VERSION 3.18)

project(ACR)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_definitions("ImDrawIdx=unsigned int")
add_compile_options(-O -g -Wall -Wextra -Wpedantic)

find_library(pigpio_LIBRARY 
	NAMES libpigpio.so
	HINTS /usr/local/lib
)
if(${pigpio_LIBRARY} STREQUAL "pigpio_LIBRARY-NOTFOUND")
	message(WARNING "pigpio not found")
	set(pigpio_LIBRARY "")
	add_compile_definitions(ACR_NO_PIGPIO)
endif()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/external/raylib)

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/inc
    ${CMAKE_CURRENT_LIST_DIR}/external
    ${CMAKE_CURRENT_LIST_DIR}/external/imgui
    ${CMAKE_CURRENT_LIST_DIR}/external/imgui/backends
    ${CMAKE_CURRENT_LIST_DIR}/external/imgui/misc/cpp
    ${CMAKE_CURRENT_LIST_DIR}/external/implot
)

add_library(acr
	STATIC
	${CMAKE_CURRENT_LIST_DIR}/src/acr.c
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

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)

add_executable(main src/main.c)
target_link_libraries(main acr gps m pthread)

find_package(GLEW      REQUIRED)
find_package(OpenGL    REQUIRED)
find_package(glfw3     REQUIRED)
add_executable(viewer
src/viewer.cpp
external/imgui/imgui.cpp
external/imgui/imgui_draw.cpp
external/imgui/imgui_widgets.cpp
external/imgui/imgui_tables.cpp
external/imgui/backends/imgui_impl_glfw.cpp
external/imgui/backends/imgui_impl_opengl2.cpp
external/implot/implot.cpp
external/implot/implot_items.cpp
)
target_link_libraries(viewer acr gps m pthread GLEW::GLEW OpenGL::GL glfw)
