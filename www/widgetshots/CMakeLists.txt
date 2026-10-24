cmake_minimum_required(VERSION 3.20)
project(fluffy_widgetshots LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

if(NOT FLUFFY_ROOT)
    get_filename_component(FLUFFY_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()
if(NOT FLUFFY_BUILD_DIR)
    set(FLUFFY_BUILD_DIR "${FLUFFY_ROOT}/build")
endif()

get_filename_component(FLUFFY_ROOT "${FLUFFY_ROOT}" ABSOLUTE)
get_filename_component(FLUFFY_BUILD_DIR "${FLUFFY_BUILD_DIR}" ABSOLUTE)

set(_fluffy_lib_dirs
    "${FLUFFY_BUILD_DIR}"
    "${FLUFFY_BUILD_DIR}/Release"
    "${FLUFFY_BUILD_DIR}/RelWithDebInfo"
    "${FLUFFY_BUILD_DIR}/Debug"
    "${FLUFFY_BUILD_DIR}/MinSizeRel"
)

find_library(FLUFFY_LIBRARY NAMES fluffy HINTS ${_fluffy_lib_dirs} NO_DEFAULT_PATH)

if(NOT FLUFFY_LIBRARY)
    message(FATAL_ERROR
        "Fluffy build artifacts not found in ${FLUFFY_BUILD_DIR}. Run ./build.sh first.")
endif()

find_path(NANOVG_INCLUDE_DIR nanovg.h
    HINTS "${FLUFFY_BUILD_DIR}/_deps/nanovg-src/src"
    NO_DEFAULT_PATH)
find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h
    HINTS "${FLUFFY_BUILD_DIR}/_deps/glfw-src/include"
    NO_DEFAULT_PATH)
find_path(WEBVIEW_INCLUDE_DIR webview/webview.h
    HINTS
        "${FLUFFY_BUILD_DIR}/_deps/webview-src/core/include"
        "${FLUFFY_BUILD_DIR}/_deps/webview-src"
    NO_DEFAULT_PATH)

if(NOT NANOVG_INCLUDE_DIR OR NOT GLFW_INCLUDE_DIR)
    message(FATAL_ERROR
        "NanoVG or GLFW headers not found in ${FLUFFY_BUILD_DIR}.")
endif()

find_package(OpenGL REQUIRED)

add_executable(widgetshots main.cpp)

target_include_directories(widgetshots PRIVATE
    "${FLUFFY_ROOT}/include"
    "${NANOVG_INCLUDE_DIR}"
    "${GLFW_INCLUDE_DIR}"
)

if(WEBVIEW_INCLUDE_DIR)
    target_include_directories(widgetshots PRIVATE "${WEBVIEW_INCLUDE_DIR}")
endif()

find_package(Threads REQUIRED)

target_link_libraries(widgetshots PRIVATE
    "${FLUFFY_LIBRARY}"
    OpenGL::GL
    Threads::Threads
)

if(APPLE)
    target_link_libraries(widgetshots PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreFoundation"
        "-framework WebKit"
    )
elseif(WIN32)
    target_link_libraries(widgetshots PRIVATE
        ole32 oleaut32 shlwapi version user32 gdi32 shell32 advapi32
    )
    set_target_properties(widgetshots PROPERTIES WIN32_EXECUTABLE TRUE)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GDK_PIXBUF REQUIRED IMPORTED_TARGET gdk-pixbuf-2.0)
    pkg_check_modules(WEBKIT2GTK QUIET IMPORTED_TARGET webkit2gtk-4.1)
    pkg_check_modules(WAYLAND QUIET IMPORTED_TARGET
        wayland-client wayland-cursor wayland-egl xkbcommon)

    target_link_libraries(widgetshots PRIVATE
        ${CMAKE_DL_LIBS}
        m
        PkgConfig::GDK_PIXBUF
    )

    foreach(_xlib X11 Xrandr Xi Xcursor Xinerama Xext)
        find_library(${_xlib}_LIBRARY NAMES ${_xlib})
        if(${_xlib}_LIBRARY)
            target_link_libraries(widgetshots PRIVATE "${${_xlib}_LIBRARY}")
        endif()
    endforeach()

    find_library(RT_LIBRARY rt)
    if(RT_LIBRARY)
        target_link_libraries(widgetshots PRIVATE "${RT_LIBRARY}")
    endif()

    if(WEBKIT2GTK_FOUND)
        target_link_libraries(widgetshots PRIVATE PkgConfig::WEBKIT2GTK)
    endif()
    if(WAYLAND_FOUND)
        target_link_libraries(widgetshots PRIVATE PkgConfig::WAYLAND)
    endif()
endif()
