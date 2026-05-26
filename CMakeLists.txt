cmake_minimum_required(VERSION 3.18)
project(ACR)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -std=c++2a -DGLEW_STATIC -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-padded -Wno-error=reorder -Wno-error=unused-function -Wno-error=unused-variable")

include_directories(SYSTEM ${GLEW_INCLUDE_DIRS})

add_compile_definitions("ImDrawIdx=unsigned int")
add_compile_definitions(GL_SILENCE_DEPRECATION)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW_PKG QUIET glfw3)
if(GLFW_PKG_FOUND)
    set(GLFW_INCLUDE_DIRS ${GLFW_PKG_INCLUDE_DIRS})
    set(GLFW_LIBRARIES ${GLFW_PKG_LIBRARIES})
endif()
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)

if(NOT GLEW_LIBRARIES)
    if(GLEW_LIBRARY_DEBUG AND GLEW_LIBRARY_RELEASE)
        set(GLEW_LIBRARIES "optimized;${GLEW_LIBRARY_RELEASE};debug;${GLEW_LIBRARY_DEBUG}")
    else()
        set(GLEW_LIBRARIES GLEW::GLEW)
    endif()
endif()

# Trova libreria pigpio
find_library(pigpio_LIBRARY
    NAMES pigpio
    HINTS /usr/local/lib
)
if(NOT pigpio_LIBRARY)
  message(WARNING "pigpio non trovato, disabilitando pigpio")
  set(pigpio_LIBRARY "")
  add_compile_definitions(ACR_NO_PIGPIO)
endif()

include_directories(
    ${GLFW_INCLUDE_DIRS}
    ${CMAKE_CURRENT_LIST_DIR}/inc
    ${CMAKE_CURRENT_LIST_DIR}/external
    ${CMAKE_CURRENT_LIST_DIR}/external/nativefiledialog-extended/src/include
    ${CMAKE_CURRENT_LIST_DIR}/external/imgui
    ${CMAKE_CURRENT_LIST_DIR}/external/imgui/misc/cpp
    ${CMAKE_CURRENT_LIST_DIR}/external/imgui/backends
    ${CMAKE_CURRENT_LIST_DIR}/external/implot
)

add_definitions(${GLIB_CFLAGS_OTHER} ${GTK_CFLAGS_OTHER})

# Aggiungi il sottodirectory di nativefiledialog-extended
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/external/nativefiledialog-extended)

# Libreria acr
add_library(acr STATIC
    ${CMAKE_CURRENT_LIST_DIR}/src/acr.c
    ${CMAKE_CURRENT_LIST_DIR}/src/led.c
    ${CMAKE_CURRENT_LIST_DIR}/src/gpio.c
    ${CMAKE_CURRENT_LIST_DIR}/src/utils.c
)
target_include_directories(acr PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/inc
    ${CMAKE_CURRENT_LIST_DIR}/external
)
target_compile_options(acr PRIVATE -O -g -Wall -Wextra)

# Libreria gps
add_library(gps STATIC
    ${CMAKE_CURRENT_LIST_DIR}/external/gpslib/gps.c
    ${CMAKE_CURRENT_LIST_DIR}/external/gpslib/gps_interface.c
)
target_include_directories(gps PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/external/gpslib
)
target_compile_options(gps PRIVATE -O -g -Wall -Wextra)

# Directory per eseguibili
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)

# Eseguibile main
add_executable(main src/main.c)
target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/inc
)
target_compile_options(main PRIVATE -O -g -Wall -Wextra)
target_link_libraries(main acr gps pthread)

FILE(GLOB imgui_srcs ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/*)
FILE(GLOB implot_srcs ${CMAKE_CURRENT_SOURCE_DIR}/external/implot/*)

add_library(
    imgui
    STATIC

    # GUI
    ${imgui_srcs}
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends/imgui_impl_opengl2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/misc/cpp/imgui_stdlib.cpp
)
target_compile_options(imgui PRIVATE -w)

add_library(
    implot
    STATIC

    # PLOT
    ${implot_srcs}
    ${CMAKE_CURRENT_SOURCE_DIR}/external/implot/implot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external/implot/implot_items.cpp
)
target_compile_options(implot PRIVATE -w)

add_library(gps_utils STATIC
    src/gps.cpp
    src/map.cpp
    src/cones_loader.cpp
)

target_include_directories(gps_utils PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/inc
)

target_link_libraries(gps_utils acr gps imgui implot)

target_compile_options(gps_utils PRIVATE -O -g -Wall -Wextra)

add_library(utils STATIC
src/profiler.cpp
src/utils.cpp
src/font_manager.cpp
src/icon_manager.cpp
src/file_browser.cpp
src/notifications.cpp
)

target_include_directories(utils PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/inc
)
target_compile_options(utils PRIVATE -O -g -Wall -Wextra)

target_link_libraries(utils gps_utils imgui implot)


add_executable(viewer src/viewer.cpp src/gui.cpp)

# Includi directory necessarie per il viewer
target_include_directories(viewer PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/inc
    ${CMAKE_CURRENT_LIST_DIR}/external
    ${CMAKE_CURRENT_LIST_DIR}/external/imgui
    ${CMAKE_CURRENT_LIST_DIR}/external/imgui/backends
    ${CMAKE_CURRENT_LIST_DIR}/external/imgui/misc/cpp
    ${CMAKE_CURRENT_LIST_DIR}/external/implot
    ${CMAKE_CURRENT_LIST_DIR}/external/nativefiledialog-extended/src/include
)

# Imposta opzioni di compilazione per viewer
target_compile_options(viewer PRIVATE -O -g -Wall -Wextra)

# Linka librerie necessarie per viewer
target_link_libraries(viewer
    acr
    gps
    m
    pthread
    OpenGL::GL
    nfd
    imgui
    implot
    gps_utils
    utils
    glfw
    OpenGL::GL
    ${GLEW_LIBRARIES}
    GLEW::GLEW
)

