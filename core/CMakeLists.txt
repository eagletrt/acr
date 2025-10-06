message("[CORE]")

set(CDIR ${CMAKE_CURRENT_LIST_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DIR}/bin/core)

configure_file(${CDIR}/src/config.hpp.in ${CDIR}/src/config.hpp)

include_directories(${CDIR}/src)
include_directories(SYSTEM ${EXTERNAL_DIRS})


#--- ACR ---#
add_library(acr STATIC)
target_sources(acr PRIVATE
  ${CDIR}/src/acr/acr.c
  ${CDIR}/src/acr/led.c
  ${CDIR}/src/acr/gpio.c
  ${CDIR}/src/acr/utils.c
)
target_include_directories(acr PUBLIC
  ${CDIR}/src
  ${CDIR}/src/acr
  ${GLIB_INCLUDE_DIRS}
  ${GTK_INCLUDE_DIRS}
  ${EXTERNAL_DIRS}
)
target_link_libraries(acr PUBLIC ${pigpio_LIBRARY} ${GLIB_LIBRARIES} ${GTK_LIBRARIES})
target_compile_options(acr PRIVATE -O -g -Wall -Wextra)

#--- GPS-UTILS ---#
add_library(gps-utils STATIC)
target_sources(gps-utils PRIVATE
  ${CDIR}/src/gps_utils/gps.cpp
  ${CDIR}/src/gps_utils/map.cpp
  ${CDIR}/src/gps_utils/cones_loader.cpp
)
target_include_directories(gps-utils PUBLIC
  ${CDIR}/src
  ${CDIR}/src/utils
  ${CDIR}/executables/main
)
target_link_libraries(gps-utils acr utils gps-lib imgui implot)
target_compile_options(gps-utils PRIVATE -O -g -Wall -Wextra)

#--- UTILS ---#
add_library(utils STATIC)
target_sources(utils PRIVATE
  ${CDIR}/src/utils/profiler.cpp
  ${CDIR}/src/utils/utils.cpp
  ${CDIR}/src/utils/font_manager.cpp
  ${CDIR}/src/utils/icon_manager.cpp
  ${CDIR}/src/utils/file_browser.cpp
  ${CDIR}/src/utils/notifications.cpp
)

target_include_directories(utils PUBLIC
  ${CDIR}/src
  ${CDIR}/src/gps_utils
  ${CDIR}/executables/main
)
target_link_libraries(utils gps-utils imgui implot)
target_compile_options(utils PRIVATE -O -g -Wall -Wextra)


#--- EXECUTABLES ---#

add_executable(main executables/main/main.c)
target_include_directories(main PRIVATE
  ${CDIR}/executables/main
  ${CDIR}/src
  ${GLIB_INCLUDE_DIRS}
  ${GTK_INCLUDE_DIRS}
  ${EXTERNAL_DIRS}
)
target_link_libraries(main acr gps-lib m pthread ${GLIB_LIBRARIES} ${GTK_LIBRARIES})
target_compile_options(main PRIVATE -O -g -Wall -Wextra)

add_executable(viewer executables/viewer/viewer.cpp executables/viewer/gui.cpp)

# Includi directory necessarie per il viewer
target_include_directories(viewer PRIVATE
    ${CDIR}/executables/viewer
    ${CDIR}/src
    ${EXTERNAL_DIRS}
    ${GLFW_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${GTK_INCLUDE_DIRS}
)

# Imposta opzioni di compilazione per viewer
target_compile_options(viewer PRIVATE -O -g -Wall -Wextra)

# Linka librerie necessarie per viewer
target_link_libraries(viewer
    acr
    gps-lib
    m
    pthread
    OpenGL::GL
    ${GLFW_LIBRARIES}  # Assicurati di usare GLFW_LIBRARIES da pkg_check_modules
    nfd
    ${GLIB_LIBRARIES}
    ${GTK_LIBRARIES}
    ${GLX_LIBRARY}
    imgui
    implot
    gps-utils
    utils
)
