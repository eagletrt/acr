cmake_minimum_required(VERSION 3.18)
project(ACR)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_definitions("ImDrawIdx=unsigned int")

find_package(PkgConfig REQUIRED)

pkg_check_modules(GLFW REQUIRED glfw3)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GTK REQUIRED gtk+-3.0)

find_library(pigpio_LIBRARY 
    NAMES pigpio
    HINTS /usr/local/lib
)
if(NOT pigpio_LIBRARY)
    message(WARNING "pigpio not found")
    set(pigpio_LIBRARY "")
    add_compile_definitions(ACR_NO_PIGPIO)
endif()

include_directories(
    ${GLIB_INCLUDE_DIRS}
    ${GTK_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    ${CMAKE_CURRENT_LIST_DIR}/inc
    ${CMAKE_CURRENT_LIST_DIR}/external   
    ${CMAKE_CURRENT_LIST_DIR}/external/nativefiledialog-extended/src/include
)

add_definitions(${GLIB_CFLAGS_OTHER} ${GTK_CFLAGS_OTHER})

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/external/nativefiledialog-extended)

add_library(acr
    STATIC
    ${CMAKE_CURRENT_LIST_DIR}/src/acr.c
    ${CMAKE_CURRENT_LIST_DIR}/src/led.c
    ${CMAKE_CURRENT_LIST_DIR}/src/gpio.c
    ${CMAKE_CURRENT_LIST_DIR}/src/utils.c
)
target_include_directories(acr PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/inc
    ${CMAKE_CURRENT_LIST_DIR}/external  
    ${GLIB_INCLUDE_DIRS}
    ${GTK_INCLUDE_DIRS}
)
target_compile_options(acr PRIVATE -O -g -Wall -Wextra)
target_link_libraries(acr PUBLIC ${pigpio_LIBRARY} ${GLIB_LIBRARIES} ${GTK_LIBRARIES})

add_library(gps
    STATIC
    ${CMAKE_CURRENT_LIST_DIR}/external/gpslib/gps.c
    ${CMAKE_CURRENT_LIST_DIR}/external/gpslib/gps_interface.c
)
target_include_directories(gps PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/external/gpslib
    ${GLIB_INCLUDE_DIRS}
    ${GTK_INCLUDE_DIRS}
)
target_compile_options(gps PRIVATE -O -g -Wall -Wextra)
target_link_libraries(gps PUBLIC ${GLIB_LIBRARIES} ${GTK_LIBRARIES})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)

add_executable(main src/main.c)
target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/inc
    ${GLIB_INCLUDE_DIRS}
    ${GTK_INCLUDE_DIRS}
)
target_compile_options(main PRIVATE -O -g -Wall -Wextra)
target_link_libraries(main acr gps m pthread ${GLIB_LIBRARIES} ${GTK_LIBRARIES})

find_package(OpenGL REQUIRED)
find_library(GLX_LIBRARY GLX)

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
    external/imgui/imgui_demo.cpp
    external/implot/implot_demo.cpp
)

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
    ${GLIB_INCLUDE_DIRS}
    ${GTK_INCLUDE_DIRS}
)

target_compile_options(viewer PRIVATE -O -g -Wall -Wextra)
target_link_libraries(viewer acr gps m pthread OpenGL::GL glfw nfd ${GLIB_LIBRARIES} ${GTK_LIBRARIES} ${GLX_LIBRARY})


