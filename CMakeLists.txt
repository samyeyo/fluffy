cmake_minimum_required(VERSION 3.20)
project(fluffy
    VERSION     0.2.0
    DESCRIPTION "A modern, multiplatform NanoVG-based GUI engine"
    LANGUAGES   C CXX
)

# ----------------------------------------------------------------------------
# Global compile settings (C++20, no non-standard extensions)
# ----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ----------------------------------------------------------------------------
# Size-optimisation flags
#
# Fluffy targets small binaries.  We: emit one section per function/data
# item so the linker can discard unused pieces (-fdata-sections), hide
# internal symbols (-fvisibility=hidden), and strip every unreferenced
# section at link time (-dead_strip).  These apply to ALL build types so
# even Debug builds only pay for symbols they actually reference.
#
# Release builds additionally override -O3 with -Oz/+LTO for maximal size
# reduction.
# ----------------------------------------------------------------------------
# Structural flags: apply to every build type so unused data (icons, etc.)
# is always discarded at the section level.  Flag spelling differs per toolchain:
# MSVC wants /Gy /Gw + /OPT:REF, GNU ld wants --gc-sections, Apple ld wants
# -dead_strip.  Applying the wrong spelling breaks the build (or, for GNU ld,
# silently misparses -dead_strip as -d -e ad_strip).
if(MSVC)
    string(APPEND CMAKE_CXX_FLAGS " /Gy /Gw /Zc:inline")
    string(APPEND CMAKE_C_FLAGS   " /Gy /Gw /Zc:inline")
    string(APPEND CMAKE_EXE_LINKER_FLAGS    " /OPT:REF /OPT:ICF")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS " /OPT:REF /OPT:ICF")

    string(REPLACE "/O2" "/O1" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/O2" "/O1" CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}")
    string(REPLACE "/Ob2" "/Ob1" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/Ob2" "/Ob1" CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}")
    string(APPEND CMAKE_CXX_FLAGS_RELEASE " /GL")
    string(APPEND CMAKE_C_FLAGS_RELEASE   " /GL")
    string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE    " /LTCG /OPT:REF /OPT:ICF")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_RELEASE " /LTCG /OPT:REF /OPT:ICF")
    string(APPEND CMAKE_STATIC_LINKER_FLAGS_RELEASE " /LTCG")
else()
    string(APPEND CMAKE_CXX_FLAGS " -ffunction-sections -fdata-sections -fvisibility=hidden")
    string(APPEND CMAKE_C_FLAGS   " -ffunction-sections -fdata-sections -fvisibility=hidden")
    if(NOT EMSCRIPTEN)
        if(APPLE)
            string(APPEND CMAKE_EXE_LINKER_FLAGS    " -Wl,-dead_strip")
            string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,-dead_strip")
        else()
            string(APPEND CMAKE_EXE_LINKER_FLAGS    " -Wl,--gc-sections")
            string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--gc-sections")
        endif()
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    # -Oz yields smaller code than -Os (Clang/AppleClang).
    # GCC only supports -Os.
    if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang")
        string(REPLACE "-O3" "-Oz" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
        string(REPLACE "-O3" "-Oz" CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        string(REPLACE "-O3" "-Os" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
        string(REPLACE "-O3" "-Os" CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}")
    endif()

    # Thin LTO — AppleClang prefers -flto=thin; GCC parallelises with
    # -flto=auto; other Clang builds use plain -flto.
    if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
        string(APPEND CMAKE_CXX_FLAGS_RELEASE " -flto=thin")
        string(APPEND CMAKE_C_FLAGS_RELEASE   " -flto=thin")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        string(APPEND CMAKE_CXX_FLAGS_RELEASE " -flto=auto")
        string(APPEND CMAKE_C_FLAGS_RELEASE   " -flto=auto")
    else()
        string(APPEND CMAKE_CXX_FLAGS_RELEASE " -flto")
        string(APPEND CMAKE_C_FLAGS_RELEASE   " -flto")
    endif()
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ----------------------------------------------------------------------------
# User-facing options
# ----------------------------------------------------------------------------
option(FLUFFY_BUILD_EXAMPLES "Build Fluffy examples" OFF)
option(FLUFFY_VERBOSE_DEPS    "Print extra information about dependency configuration" OFF)
option(FLUFFY_NO_RTTI         "Disable C++ RTTI (-fno-rtti) for smaller binaries" ON)
option(FLUFFY_NO_EXCEPTIONS  "Disable C++ exceptions (-fno-exceptions) for smaller binaries" ON)

# ----------------------------------------------------------------------------
# Dependencies (NanoVG + GLFW + gl3w + OpenGL)
# ----------------------------------------------------------------------------
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/BuildDeps.cmake)

# ----------------------------------------------------------------------------
# Lucide SVG icons — download individual icon SVGs and generate C++ data file
# ----------------------------------------------------------------------------
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadLucideSvgs.cmake)

# ----------------------------------------------------------------------------
# WebView overlay — webview/webview with built-in WebView2 loader (auto NuGet on Win)
# FLUFFY_ENABLE_WEBVIEW ON by default, LTO DCE ensures zero cost when unused.
# ----------------------------------------------------------------------------
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchWebview.cmake)

# ----------------------------------------------------------------------------
# Fluffy library
# ----------------------------------------------------------------------------
set(_fluffy_sources
    src/theme.cpp
    src/widget.cpp
    src/label.cpp
    src/button.cpp
    src/checkbox.cpp
    src/radiobutton.cpp
    src/slider.cpp
    src/window.cpp
    src/application.cpp
    src/dispatcher.cpp
    src/factory.cpp
    src/layout.cpp
    src/container.cpp
    src/constraint.cpp
    src/yaml_loader.cpp
    src/svg_icon.cpp
    src/modal_window.cpp
    src/switch.cpp
    src/render.cpp
    src/textinput.cpp
    src/color_picker.cpp
    src/menu.cpp
    src/nav.cpp
    src/image.cpp
    src/font.cpp
    src/combobox.cpp
    src/listview.cpp
    src/tabview.cpp
    src/table.cpp
    src/progress_ring.cpp
    src/simple_widget.cpp
    src/tooltip.cpp
    third_party/dyml/dyml_impl.cpp
)

# Platform-specific image decoder.
if(EMSCRIPTEN)
    # Emscripten: use stub (no file decoding, only RGBA pixels)
elseif(APPLE)
    list(APPEND _fluffy_sources src/image_mac.cpp)
elseif(WIN32)
    list(APPEND _fluffy_sources src/image_win.cpp)
else()
    list(APPEND _fluffy_sources src/image_linux.cpp)
endif()

# If the SVG data was generated, add it to the library sources.
if(FLUFFY_LUCIDE_SVG_DATA)
    list(APPEND _fluffy_sources "${FLUFFY_LUCIDE_SVG_DATA}")
    message(STATUS "Fluffy: Lucide SVG icon data added to library")
endif()

# WebView overlay sources — always include common stub (LTO DCE when unused),
# platform impl only when enabled. Disabled on Emscripten (no native overlay).
if(EMSCRIPTEN)
    set(FLUFFY_WEBVIEW_ENABLED OFF)
endif()
list(APPEND _fluffy_sources src/webview.cpp)
if(FLUFFY_WEBVIEW_ENABLED)
    if(APPLE)
        list(APPEND _fluffy_sources src/webview_mac.cpp)
    elseif(WIN32)
        list(APPEND _fluffy_sources src/webview_win.cpp)
    else()
        list(APPEND _fluffy_sources src/webview_linux.cpp)
    endif()
endif()

# Emscripten: WebGL2 + GLFW from Emscripten ports
if(EMSCRIPTEN)
    add_compile_definitions(FLUFFY_EMSCRIPTEN=1)
    set(CMAKE_EXECUTABLE_SUFFIX ".js")
    # Use GLES2 backend for NanoVG on WebGL
    add_compile_definitions(NANOVG_GLES2)
endif()

add_library(fluffy ${_fluffy_sources}
    $<TARGET_OBJECTS:fluffy_nanovg_obj>
)
if(NOT EMSCRIPTEN)
    target_sources(fluffy PRIVATE
        $<TARGET_OBJECTS:glfw>
        $<TARGET_OBJECTS:fluffy_gl3w_obj>
    )
endif()

if(CMAKE_CONFIGURATION_TYPES)
    foreach(_config IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER "${_config}" _config_upper)
        set_target_properties(fluffy PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY_${_config_upper} "${CMAKE_CURRENT_BINARY_DIR}"
            LIBRARY_OUTPUT_DIRECTORY_${_config_upper} "${CMAKE_CURRENT_BINARY_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_${_config_upper}  "${CMAKE_CURRENT_BINARY_DIR}")
    endforeach()
endif()

# Consumers must compile as C++20 too: public headers use std::optional,
# std::clamp, std::span-style APIs and concepts.  Propagate the requirement
# through the target so find_package(Fluffy) users inherit it automatically.
target_compile_features(fluffy PUBLIC cxx_std_20)

target_include_directories(fluffy
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Third-party headers are included as SYSTEM so their warnings don't leak
# into Fluffy's -Wall -Wextra -Wpedantic build.
target_include_directories(fluffy SYSTEM PUBLIC
    $<BUILD_INTERFACE:${nanovg_SOURCE_DIR}/src>
)

target_include_directories(fluffy SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nanosvg
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/gl3w
)
if(NOT EMSCRIPTEN)
    target_include_directories(fluffy PUBLIC
        $<BUILD_INTERFACE:${glfw_SOURCE_DIR}/include>)
endif()

if(EMSCRIPTEN)
    target_compile_definitions(fluffy PRIVATE NANOVG_GLES2)
else()
    target_compile_definitions(fluffy PRIVATE NANOVG_GL3)
endif()

target_link_libraries(fluffy
    PUBLIC
        OpenGL::GL
        ${FLUFFY_GLFW_LINK_LIBRARIES}
)

# Emscripten WebGL flags for the object files and final application.
if(EMSCRIPTEN)
    target_compile_options(fluffy PRIVATE "SHELL:-s USE_GLFW=3" "SHELL:-s USE_WEBGL2=1")
    target_compile_options(fluffy_nanovg_obj PRIVATE "SHELL:-s USE_GLFW=3" "SHELL:-s USE_WEBGL2=1")
    target_link_options(fluffy INTERFACE "SHELL:-s USE_GLFW=3" "SHELL:-s USE_WEBGL2=1" "SHELL:-s WASM=1" "SHELL:-s ALLOW_MEMORY_GROWTH=1" "SHELL:-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']" "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/assets/Roboto-Regular.ttf@/assets/Roboto-Regular.ttf" "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/assets/Roboto-Bold.ttf@/assets/Roboto-Bold.ttf" "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/assets/LICENSE-Roboto.txt@/assets/LICENSE-Roboto.txt")
endif()

# svg_icon.cpp includes the ~3000-line nanosvg.h implementation.
# Always compile it with size optimization to keep the binary lean,
# even in Debug builds where the default -O0 would bloat the parser.
set_source_files_properties(src/svg_icon.cpp PROPERTIES COMPILE_OPTIONS
    "$<$<CONFIG:Debug,RelWithDebInfo>:-Os>"
)

if(FLUFFY_WEBVIEW_ENABLED)
    target_compile_definitions(fluffy PUBLIC FLUFFY_ENABLE_WEBVIEW=1)
    if(TARGET fluffy_webview)
        target_link_libraries(fluffy PUBLIC fluffy_webview)
    endif()
    if(TARGET fluffy_webview_sdk)
        target_link_libraries(fluffy PUBLIC fluffy_webview_sdk)
    endif()
endif()

# Linux: gdk-pixbuf for image decoding, and core X11 used directly by
# window.cpp (GLFW's X11 backend dlopen()s its own libraries).
if(NOT APPLE AND NOT WIN32 AND NOT EMSCRIPTEN)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(gdk_pixbuf REQUIRED IMPORTED_TARGET gdk-pixbuf-2.0)
    target_link_libraries(fluffy PRIVATE PkgConfig::gdk_pixbuf)

    find_package(X11 REQUIRED)
    target_link_libraries(fluffy PUBLIC X11::X11)
    target_link_libraries(fluffy PRIVATE ${CMAKE_DL_LIBS})
endif()

# Sensible warnings-as-warnings (not errors) for our own sources.
target_compile_options(fluffy PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    target_compile_options(fluffy PUBLIC -Wno-dollar-in-identifier-extension)
endif()

# Optional compiler knobs for smaller binaries.
# These are PRIVATE: they shape how the library itself is compiled but must
# not force -fno-rtti / -fno-exceptions onto consumer translation units.
if(FLUFFY_NO_RTTI)
    # The TUs that own the vtable/typeinfo of public polymorphic bases
    # (Widget, Layout, SimpleWidget) must keep RTTI: consumers derive from
    # those bases with RTTI enabled, and their typeinfo references the base
    # typeinfo emitted in the key-function TU.  Everything else is compiled
    # -fno-rtti.
    set(_fluffy_no_rtti_sources ${_fluffy_sources})
    list(REMOVE_ITEM _fluffy_no_rtti_sources
        src/widget.cpp src/layout.cpp src/simple_widget.cpp)
    set_source_files_properties(${_fluffy_no_rtti_sources} PROPERTIES
        COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fno-rtti>$<$<CXX_COMPILER_ID:MSVC>:/GR->")
    message(STATUS "Fluffy: RTTI disabled (-fno-rtti), Widget/Layout TUs keep RTTI")
endif()

# Disable errno math and unwind tables when exceptions are off.
# These shave a few extra KB off the binary.
if(FLUFFY_NO_EXCEPTIONS)
    target_compile_options(fluffy PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables>
    )
    message(STATUS "Fluffy: exceptions disabled (-fno-exceptions)")
endif()

# NOTE: -fno-threadsafe-statics is intentionally NOT used — Application::get()
# is a Meyer's singleton whose guard variable must stay thread-safe.
target_compile_options(fluffy PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fno-math-errno>
)

# ----------------------------------------------------------------------------
# Examples
# ----------------------------------------------------------------------------
if(FLUFFY_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()


# ----------------------------------------------------------------------------
# Install + CMake package export (find_package(Fluffy))
# ----------------------------------------------------------------------------
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Canonical name inside the exported FluffyTargets.cmake: fluffy::fluffy.
# GLFW, NanoVG and gl3w are internal object files in libfluffy.a, not public libs.
set(_fluffy_export_targets fluffy)
if(TARGET fluffy_webview)
    list(APPEND _fluffy_export_targets fluffy_webview)
endif()
if(TARGET fluffy_webview_sdk)
    list(APPEND _fluffy_export_targets fluffy_webview_sdk)
endif()
install(TARGETS ${_fluffy_export_targets}
    EXPORT  FluffyTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(EXPORT FluffyTargets
    NAMESPACE fluffy::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Fluffy
)
configure_package_config_file(
    cmake/FluffyConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/FluffyConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Fluffy
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/FluffyConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/FluffyConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/FluffyConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Fluffy
)
install(DIRECTORY include/fluffy DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
# Public Fluffy headers (svg_icon.hpp, simple_widget.hpp, icons.hpp) include
# <nanovg.h>, so the installed package must ship the matching header too.
install(FILES "${nanovg_SOURCE_DIR}/src/nanovg.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
# GLFW is linked into libfluffy.a, but its public header is still needed by
# applications that interact with GLFWwindow (Window::glfwWindow()).
if(NOT EMSCRIPTEN)
    install(DIRECTORY "${glfw_SOURCE_DIR}/include/GLFW"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(FILES "${glfw_SOURCE_DIR}/LICENSE.md"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/fluffy/third_party/glfw)
endif()

# ----------------------------------------------------------------------------
# Bundled Lucide font attribution — required by Lucide's ISC license.
# cmake/FetchLucide.cmake downloads lucide.ttf from
# https://cdn.jsdelivr.net/npm/lucide-static (ISC, permissive) into
# ${CMAKE_BINARY_DIR}/third_party/fluffy_lucide/, and the bundled-font
# examples register those bytes via Window::register_font_memory.
# Shipping compiled binaries that embed upstream TTF bytes requires the
# upstream licence + attribution to be visible somewhere — we ship a
# copy under the source tree so it follows the project via Git / tarball
# / install (third_party/fluffy_lucide/LICENSE.txt).
# ----------------------------------------------------------------------------
install(DIRECTORY third_party/fluffy_lucide
    DESTINATION ${CMAKE_INSTALL_DATADIR}/fluffy/third_party
    FILES_MATCHING PATTERN "*.txt" PATTERN "*.md")

# ----------------------------------------------------------------------------
# Bundled Roboto font attribution — required by Apache-2.0.  The TTFs under
# assets/ are embedded verbatim into WebAssembly builds (see the Emscripten
# --preload-file options above), so binaries built from those builds carry
# upstream font data and must ship the licence text with them.  The same copy
# is preloaded into the WASM virtual filesystem at /assets/ and installed here
# so it follows the project via Git / tarball / install.
# ----------------------------------------------------------------------------
install(FILES assets/LICENSE-Roboto.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/fluffy)

message(STATUS "")
message(STATUS "Fluffy ${PROJECT_VERSION} configured.")
message(STATUS "  C++ compiler:  ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Build type:    ${CMAKE_BUILD_TYPE}")
message(STATUS "  Examples:      ${FLUFFY_BUILD_EXAMPLES}")
message(STATUS "")
