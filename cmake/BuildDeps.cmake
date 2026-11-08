include(FetchContent)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
set(FETCHCONTENT_QUIET FALSE)

if(NOT EMSCRIPTEN)
    find_package(Threads REQUIRED)
    set(FLUFFY_GLFW_VERSION "3.4" CACHE STRING "GLFW release tag to fetch.")

    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
    set(GLFW_LIBRARY_TYPE   OBJECT CACHE STRING "" FORCE)
    set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        glfw
        URL https://github.com/glfw/glfw/archive/refs/tags/${FLUFFY_GLFW_VERSION}.tar.gz
    )
    FetchContent_GetProperties(glfw)
    if(NOT glfw_POPULATED)
        FetchContent_MakeAvailable(glfw)
    endif()

    if(NOT TARGET glfw)
        message(FATAL_ERROR "GLFW target not produced by FetchContent.")
    endif()

    get_target_property(FLUFFY_GLFW_LINK_LIBRARIES glfw INTERFACE_LINK_LIBRARIES)
    if(NOT FLUFFY_GLFW_LINK_LIBRARIES)
        set(FLUFFY_GLFW_LINK_LIBRARIES "")
    endif()
endif()

set(FLUFFY_NANOVG_BRANCH "master" CACHE STRING "NanoVG branch / tag on memononen/nanovg.")

FetchContent_Declare(
    nanovg
    URL https://github.com/memononen/nanovg/archive/${FLUFFY_NANOVG_BRANCH}.tar.gz
)
FetchContent_GetProperties(nanovg)
if(NOT nanovg_POPULATED)
    FetchContent_MakeAvailable(nanovg)
endif()

# stb_truetype 1.26 replaces the 1.24 copy bundled with NanoVG.
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb/stb_truetype.h"
     DESTINATION "${nanovg_SOURCE_DIR}/src")

if(NOT TARGET fluffy_nanovg_obj)
    add_library(fluffy_nanovg_obj OBJECT
        "${nanovg_SOURCE_DIR}/src/nanovg.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/nanovg_gl3.c"
    )
    target_include_directories(fluffy_nanovg_obj PRIVATE
        "${nanovg_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/gl3w")
    if(MSVC)
        target_compile_definitions(fluffy_nanovg_obj PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
    if(EMSCRIPTEN)
        target_compile_definitions(fluffy_nanovg_obj PRIVATE
            NANOVG_GLES2_IMPLEMENTATION NVG_NO_STB)
        target_compile_options(fluffy_nanovg_obj PRIVATE
            "SHELL:-include GLES2/gl2.h")
    else()
        target_compile_definitions(fluffy_nanovg_obj PRIVATE
            NANOVG_GL3_IMPLEMENTATION NVG_NO_STB)
        # Force-include gl3w.h so every GL call goes through gl3wprocs.
        if(MSVC)
            target_compile_options(fluffy_nanovg_obj PRIVATE "/FIgl3w.h")
        else()
            target_compile_options(fluffy_nanovg_obj PRIVATE "SHELL:-include gl3w.h")
        endif()
    endif()
endif()

if(NOT EMSCRIPTEN)
    if(NOT TARGET fluffy_gl3w_obj)
        add_library(fluffy_gl3w_obj OBJECT
            "${CMAKE_CURRENT_SOURCE_DIR}/third_party/gl3w/gl3w.c")
        target_include_directories(fluffy_gl3w_obj PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/third_party/gl3w")
    endif()
endif()

if(EMSCRIPTEN)
    if(NOT TARGET OpenGL::GL)
        add_library(OpenGL::GL INTERFACE IMPORTED GLOBAL)
    endif()
else()
    find_package(OpenGL REQUIRED)
    if(NOT TARGET OpenGL::GL)
        add_library(OpenGL::GL UNKNOWN IMPORTED)
        set_target_properties(OpenGL::GL PROPERTIES
            IMPORTED_LOCATION "${OPENGL_gl_LIBRARY}")
    endif()
endif()
