cmake_minimum_required(VERSION 3.18)
project(ACR)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(DIR ${CMAKE_CURRENT_LIST_DIR})
set(CDIR ${CMAKE_CURRENT_LIST_DIR})

add_compile_definitions("ImDrawIdx=unsigned int")

find_package(PkgConfig REQUIRED)

pkg_check_modules(GLFW REQUIRED glfw3)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GTK REQUIRED gtk+-3.0)

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
  ${GLIB_INCLUDE_DIRS}
  ${GTK_INCLUDE_DIRS}
  ${GLFW_INCLUDE_DIRS}
)

find_package(OpenGL REQUIRED)
find_library(GLX_LIBRARY GLX)

add_definitions(${GLIB_CFLAGS_OTHER} ${GTK_CFLAGS_OTHER})

add_subdirectory(${CDIR}/external)
add_subdirectory(${CDIR}/core)
