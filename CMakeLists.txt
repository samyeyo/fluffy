cmake_minimum_required(VERSION 3.20)
project(fluffy
    VERSION     0.2.0
    DESCRIPTION "A modern, multiplatform NanoVG-based GUI engine"
    LANGUAGES   C CXX
)

set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# clang-cl reports compiler ID "Clang" with an MSVC ABI.
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
    set(FLUFFY_CLANG_CL ON)
else()
    set(FLUFFY_CLANG_CL OFF)
endif()

if(MSVC)
    string(APPEND CMAKE_CXX_FLAGS " /Gy /Gw /Zc:inline")
    string(APPEND CMAKE_C_FLAGS   " /Gy /Gw /Zc:inline")
    string(APPEND CMAKE_EXE_LINKER_FLAGS    " /OPT:REF /OPT:ICF")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS " /OPT:REF /OPT:ICF")

    string(REPLACE "/O2" "/O1" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/O2" "/O1" CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}")
    string(REPLACE "/Ob2" "/Ob1" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/Ob2" "/Ob1" CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}")

    # clang-cl drops /GL, so its LTO is set up with -flto further down.
    if(NOT FLUFFY_CLANG_CL)
        string(APPEND CMAKE_CXX_FLAGS_RELEASE " /GL")
        string(APPEND CMAKE_C_FLAGS_RELEASE   " /GL")
        string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE    " /LTCG /OPT:REF /OPT:ICF")
        string(APPEND CMAKE_SHARED_LINKER_FLAGS_RELEASE " /LTCG /OPT:REF /OPT:ICF")
        string(APPEND CMAKE_STATIC_LINKER_FLAGS_RELEASE " /LTCG")
    endif()
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

if(FLUFFY_CLANG_CL)
    # Only lld-link reads bitcode.
    if(CMAKE_LINKER MATCHES "lld-link")
        string(APPEND CMAKE_CXX_FLAGS_RELEASE " -flto=thin")
        string(APPEND CMAKE_C_FLAGS_RELEASE   " -flto=thin")
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang")
        string(REPLACE "-O3" "-Oz" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
        string(REPLACE "-O3" "-Oz" CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        string(REPLACE "-O3" "-Os" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
        string(REPLACE "-O3" "-Os" CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE}")
    endif()

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

option(FLUFFY_BUILD_EXAMPLES "Build Fluffy examples" OFF)
option(FLUFFY_VERBOSE_DEPS    "Print extra information about dependency configuration" OFF)
option(FLUFFY_NO_RTTI         "Disable C++ RTTI (-fno-rtti) for smaller binaries" ON)
option(FLUFFY_NO_EXCEPTIONS  "Disable C++ exceptions (-fno-exceptions) for smaller binaries" ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/BuildDeps.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadLucideSvgs.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchWebview.cmake)

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

if(APPLE)
    list(APPEND _fluffy_sources src/image_mac.cpp)
elseif(WIN32)
    list(APPEND _fluffy_sources src/image_win.cpp)
elseif(NOT EMSCRIPTEN)
    list(APPEND _fluffy_sources src/image_linux.cpp)
endif()

if(FLUFFY_LUCIDE_SVG_DATA)
    list(APPEND _fluffy_sources "${FLUFFY_LUCIDE_SVG_DATA}")
    message(STATUS "Fluffy: Lucide SVG icon data added to library")
endif()

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

if(EMSCRIPTEN)
    add_compile_definitions(FLUFFY_EMSCRIPTEN=1 NANOVG_GLES2)
    set(CMAKE_EXECUTABLE_SUFFIX ".js")
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

target_compile_features(fluffy PUBLIC cxx_std_20)

target_include_directories(fluffy
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

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

if(EMSCRIPTEN)
    target_compile_options(fluffy PRIVATE "SHELL:-s USE_GLFW=3" "SHELL:-s USE_WEBGL2=1")
    target_compile_options(fluffy_nanovg_obj PRIVATE "SHELL:-s USE_GLFW=3" "SHELL:-s USE_WEBGL2=1")
    target_link_options(fluffy INTERFACE "SHELL:-s USE_GLFW=3" "SHELL:-s USE_WEBGL2=1" "SHELL:-s WASM=1" "SHELL:-s ALLOW_MEMORY_GROWTH=1" "SHELL:-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']" "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/assets/Roboto-Regular.ttf@/assets/Roboto-Regular.ttf" "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/assets/Roboto-Bold.ttf@/assets/Roboto-Bold.ttf" "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/assets/LICENSE-Roboto.txt@/assets/LICENSE-Roboto.txt")
endif()

# nanosvg is large; optimise svg_icon.cpp for size even in Debug.
if(MSVC)
    set(_fluffy_size_opt_flag "/O1")
else()
    set(_fluffy_size_opt_flag "-Os")
endif()
set_source_files_properties(src/svg_icon.cpp PROPERTIES COMPILE_OPTIONS
    "$<$<CONFIG:Debug,RelWithDebInfo>:${_fluffy_size_opt_flag}>"
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

if(NOT APPLE AND NOT WIN32 AND NOT EMSCRIPTEN)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(gdk_pixbuf REQUIRED IMPORTED_TARGET gdk-pixbuf-2.0)
    target_link_libraries(fluffy PRIVATE PkgConfig::gdk_pixbuf)

    find_package(X11 REQUIRED)
    target_link_libraries(fluffy PUBLIC X11::X11)
    target_link_libraries(fluffy PRIVATE ${CMAKE_DL_LIBS})
endif()

if(FLUFFY_CLANG_CL)
    # -Wall becomes /Wall, which clang-cl maps to -Weverything.
    target_compile_options(fluffy PRIVATE /W4)
    target_compile_definitions(fluffy PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(fluffy PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>
    )
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    target_compile_options(fluffy PUBLIC -Wno-dollar-in-identifier-extension)
endif()

if(FLUFFY_NO_RTTI)
    # These key-function TUs keep RTTI; consumer typeinfo refers to theirs.
    set(_fluffy_no_rtti_sources ${_fluffy_sources})
    list(REMOVE_ITEM _fluffy_no_rtti_sources
        src/widget.cpp src/layout.cpp src/simple_widget.cpp)
    if(MSVC)
        set(_fluffy_no_rtti_flag "/GR-")
    else()
        set(_fluffy_no_rtti_flag "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fno-rtti>")
    endif()
    # APPEND, so svg_icon.cpp keeps its /O1.
    set_property(SOURCE ${_fluffy_no_rtti_sources} APPEND PROPERTY
        COMPILE_OPTIONS "${_fluffy_no_rtti_flag}")
    message(STATUS "Fluffy: RTTI disabled (${_fluffy_no_rtti_flag}), Widget/Layout TUs keep RTTI")
endif()

if(FLUFFY_NO_EXCEPTIONS)
    if(FLUFFY_CLANG_CL)
        # clang-cl spells it /EHs-c-. Per-source, because webview/webview's
        # header uses try/throw and target-wide flags would reach it too.
        set(_fluffy_exception_sources ${_fluffy_sources})
        if(WIN32 AND FLUFFY_WEBVIEW_ENABLED)
            list(REMOVE_ITEM _fluffy_exception_sources src/webview_win.cpp)
        endif()
        set_property(SOURCE ${_fluffy_exception_sources} APPEND PROPERTY
            COMPILE_OPTIONS "/EHs-c-")
        message(STATUS "Fluffy: exceptions disabled (/EHs-c-)")
    else()
        target_compile_options(fluffy PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables>
        )
        message(STATUS "Fluffy: exceptions disabled (-fno-exceptions)")
    endif()
endif()

if(NOT FLUFFY_CLANG_CL)
    # MSVC-ABI math never sets errno.
    target_compile_options(fluffy PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fno-math-errno>
    )
endif()

if(FLUFFY_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

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
install(FILES "${nanovg_SOURCE_DIR}/src/nanovg.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
if(NOT EMSCRIPTEN)
    install(DIRECTORY "${glfw_SOURCE_DIR}/include/GLFW"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(FILES "${glfw_SOURCE_DIR}/LICENSE.md"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/fluffy/third_party/glfw)
endif()

# Licences of the embedded assets.
install(DIRECTORY third_party/fluffy_lucide
    DESTINATION ${CMAKE_INSTALL_DATADIR}/fluffy/third_party
    FILES_MATCHING PATTERN "*.txt" PATTERN "*.md")
install(FILES assets/LICENSE-Roboto.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/fluffy)

message(STATUS "")
message(STATUS "Fluffy ${PROJECT_VERSION} configured.")
message(STATUS "  C++ compiler:  ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Build type:    ${CMAKE_BUILD_TYPE}")
message(STATUS "  Examples:      ${FLUFFY_BUILD_EXAMPLES}")
message(STATUS "")
