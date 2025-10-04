message("EXTERNAL")

set(CDIR ${CMAKE_CURRENT_LIST_DIR})

set(EXTERNAL_DIRS
  ${CDIR}
  ${CDIR}/gpslib
  ${CDIR}/nativefiledialog-extended/src/include
  ${CDIR}/imgui
  ${CDIR}/imgui/misc/cpp
  ${CDIR}/imgui/backends
  ${CDIR}/implot
)

include_directories(PUBLIC ${EXTERNAL_DIRS})

#--- NATIVE FILE DIALOG ---#
add_subdirectory(${CDIR}/nativefiledialog-extended)

#--- GPSLIB ---#
add_library(gps-lib SHARED
  ${CDIR}/gpslib/gps.c
  ${CDIR}/gpslib/gps_interface.c
)
target_include_directories(gps-lib PUBLIC
  ${CDIR}/gpslib
  ${GLIB_INCLUDE_DIRS}
  ${GTK_INCLUDE_DIRS}
)
target_link_libraries(gps-lib PUBLIC ${GLIB_LIBRARIES} ${GTK_LIBRARIES})
target_compile_options(gps-lib PRIVATE -O -g -Wall -Wextra)

#--- IMGUI / IMPLOT ---#

FILE(GLOB imgui_srcs ${CDIR}/imgui/*)
FILE(GLOB implot_srcs ${CDIR}/implot/*)

add_library(
    imgui
    SHARED

    # GUI
    ${imgui_srcs}
    ${CDIR}/imgui/backends/imgui_impl_glfw.cpp
    ${CDIR}/imgui/backends/imgui_impl_opengl2.cpp
    ${CDIR}/imgui/misc/cpp/imgui_stdlib.cpp
)
target_compile_options(imgui PRIVATE -w)

add_library(
    implot
    SHARED

    # PLOT
    ${implot_srcs}
    ${CDIR}/implot/implot.cpp
    ${CDIR}/implot/implot_items.cpp
)
target_compile_options(implot PRIVATE -w)

set(EXTERNAL_DIRS ${EXTERNAL_DIRS} PARENT_SCOPE)
