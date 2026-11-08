# =============================================================================
#  BuildDeps.cmake — centralised dependency fetching / discovery for Fluffy.
#
#  Three third-party dependencies, all fetched via FetchContent or vendored:
#      * GLFW   — source archive + official CMakeLists.txt (object files)
#      * NanoVG — source archive; we compile `nanovg.c` with `NANOVG_GL3_IMPLEMENTATION`
#      * gl3w   — vendored pair in `third_party/gl3w/` (~15 KB total).
#                 Public-domain (Unlicense) GL 4.5 Core function-pointer
#                 table.  Composed with NanoVG by force-including gl3w.h,
#                 which `#define`s every `glXxx` to `gl3wProcs.gl.Xxx`.
#                 Pure runtime resolution via `glfwGetProcAddress` — works
#                 on every desktop platform with a Core profile context.
# =============================================================================

include(FetchContent)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
set(FETCHCONTENT_QUIET FALSE)

# ----------------------------------------------------------------------------
# 1. GLFW — compiled into libfluffy.a as object files.
# ----------------------------------------------------------------------------
if(NOT EMSCRIPTEN)
    find_package(Threads REQUIRED)
    set(FLUFFY_GLFW_VERSION "3.4" CACHE STRING "GLFW release tag to fetch.")

    # Reduce GLFW compile time by skipping examples, tests, and docs.  GLFW is an
    # OBJECT library so its objects can be embedded in the single libfluffy.a;
    # no separate GLFW library or CMake package is installed.
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

    # Propagate GLFW's platform link dependencies (Threads, Cocoa, IOKit, ...).
    get_target_property(FLUFFY_GLFW_LINK_LIBRARIES glfw INTERFACE_LINK_LIBRARIES)
    if(NOT FLUFFY_GLFW_LINK_LIBRARIES)
        set(FLUFFY_GLFW_LINK_LIBRARIES "")
    endif()
endif()

# ----------------------------------------------------------------------------
# 2. NanoVG — compiled into libfluffy.a as object files.
# ----------------------------------------------------------------------------
set(FLUFFY_NANOVG_BRANCH "master" CACHE STRING "NanoVG branch / tag on memononen/nanovg.")

FetchContent_Declare(
    nanovg
    URL https://github.com/memononen/nanovg/archive/${FLUFFY_NANOVG_BRANCH}.tar.gz
)
FetchContent_GetProperties(nanovg)
if(NOT nanovg_POPULATED)
    FetchContent_MakeAvailable(nanovg)
endif()

# Patch: replace NanoVG's bundled stb_truetype v1.24 with v1.26
# which fixes CFF font rendering and broken rasterizer bugs.
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb/stb_truetype.h"
     DESTINATION "${nanovg_SOURCE_DIR}/src")

if(NOT TARGET fluffy_nanovg_obj)
    # Two translation units:
    #   1. nanovg.c                  — base renderer (no GL backend)
    #   2. src/nanovg_gl3.c (ours)   — emits the GL3 backend from
    #                                  nanovg_gl.h's NANOVG_GL3_IMPLEMENTATION
    #                                  block.  Without this, nvgCreateGL3 /
    #                                  nvgDeleteGL3 would be undefined.
    #
    # OBJECT library: these objects are embedded directly into the single
    # libfluffy.a archive instead of producing a second public library.
    add_library(fluffy_nanovg_obj OBJECT
        "${nanovg_SOURCE_DIR}/src/nanovg.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/nanovg_gl3.c"
    )
    target_include_directories(fluffy_nanovg_obj PRIVATE
        "${nanovg_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/gl3w")
    # Only these implementation TUs must see the IMPLEMENTATION block.
    if(EMSCRIPTEN)
        target_compile_definitions(fluffy_nanovg_obj PRIVATE
            NANOVG_GLES2_IMPLEMENTATION NVG_NO_STB)
        target_compile_options(fluffy_nanovg_obj PRIVATE
            "SHELL:-include GLES2/gl2.h")
    else()
        target_compile_definitions(fluffy_nanovg_obj PRIVATE
            NANOVG_GL3_IMPLEMENTATION NVG_NO_STB)
        # Force-include gl3w.h so NanoVG routes every GL call through our
        # function-pointer table instead of requiring link-time GL symbols.
        if(MSVC)
            target_compile_options(fluffy_nanovg_obj PRIVATE "/FIgl3w.h")
        else()
            target_compile_options(fluffy_nanovg_obj PRIVATE "SHELL:-include gl3w.h")
        endif()
    endif()
endif()

# ----------------------------------------------------------------------------
# 3. gl3w — compiled into libfluffy.a as an object file.
#
# gl3w unconditionally routes every GL 4.x call through its function
# pointer table.  It resolves `gl3wInit2(glfwGetProcAddress)` once.
# ----------------------------------------------------------------------------
if(NOT EMSCRIPTEN)
        if(NOT TARGET fluffy_gl3w_obj)
            add_library(fluffy_gl3w_obj OBJECT
                "${CMAKE_CURRENT_SOURCE_DIR}/third_party/gl3w/gl3w.c")
            target_include_directories(fluffy_gl3w_obj PRIVATE
                "${CMAKE_CURRENT_SOURCE_DIR}/third_party/gl3w")
        endif()
endif()

# ----------------------------------------------------------------------------
# 4. OpenGL — system library.
# ----------------------------------------------------------------------------
if(EMSCRIPTEN)
    # WebGL provided by browser, no system OpenGL
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
