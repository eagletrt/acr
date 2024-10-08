cmake_minimum_required(VERSION 3.18)
project(ACR)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_definitions("ImDrawIdx=unsigned int")

# Gestione pigpio
find_library(pigpio_LIBRARY 
    NAMES libpigpio.so
    HINTS /usr/local/lib
)
if(${pigpio_LIBRARY} STREQUAL "pigpio_LIBRARY-NOTFOUND")
    message(WARNING "pigpio not found")
    set(pigpio_LIBRARY "")
    add_compile_definitions(ACR_NO_PIGPIO)
endif()

# Aggiungi la libreria nativefiledialog-extended
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/external/nativefiledialog-extended)

# Aggiunta delle librerie acr e gps
add_library(acr
    STATIC
    ${CMAKE_CURRENT_LIST_DIR}/src/acr.c
    ${CMAKE_CURRENT_LIST_DIR}/src/led.c
    ${CMAKE_CURRENT_LIST_DIR}/src/gpio.c
    ${CMAKE_CURRENT_LIST_DIR}/src/utils.c
)
target_include_directories(acr PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/inc
    ${CMAKE_CURRENT_LIST_DIR}/external  # Aggiunto per gpslib/gps_interface.h
)
target_compile_options(acr PRIVATE -O -g -Wall -Wextra)
target_link_libraries(acr PUBLIC ${pigpio_LIBRARY})

add_library(gps
    STATIC
    ${CMAKE_CURRENT_LIST_DIR}/external/gpslib/gps.c
    ${CMAKE_CURRENT_LIST_DIR}/external/gpslib/gps_interface.c
)
target_include_directories(gps PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/external/gpslib
)
target_compile_options(gps PRIVATE -O -g -Wall -Wextra)

# Imposta la directory di output degli eseguibili
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)

# Eseguibile principale
add_executable(main src/main.c)
target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/inc
)
target_compile_options(main PRIVATE -O -g -Wall -Wextra)
target_link_libraries(main acr gps m pthread)

# OpenGL
find_package(OpenGL REQUIRED)

# Find GLFW using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)

# Optionally, you can check if GLFW was found
if (GLFW_FOUND)
    message(STATUS "Found GLFW: version ${GLFW_VERSION}")
else()
    message(FATAL_ERROR "GLFW not found")
endif()

# Eseguibile viewer
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
# Specifica le include directories per il target viewer
target_include_directories(viewer PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}              
    ${CMAKE_CURRENT_LIST_DIR}/inc
    ${CMAKE_CURRENT_LIST_DIR}/external
    ${CMAKE_CURRENT_LIST_DIR}/external/imgui
    ${CMAKE_CURRENT_LIST_DIR}/external/imgui/backends
    ${CMAKE_CURRENT_LIST_DIR}/external/imgui/misc/cpp
    ${CMAKE_CURRENT_LIST_DIR}/external/implot
    ${GLFW_INCLUDE_DIRS}
    ${CMAKE_CURRENT_LIST_DIR}/external/nativefiledialog-extended/src/include
)

target_compile_options(viewer PRIVATE -O -g -Wall -Wextra)

target_link_libraries(viewer acr gps m pthread OpenGL::GL glfw nfd)
find_package(OpenGL REQUIRED COMPONENTS OpenGL GLX)
