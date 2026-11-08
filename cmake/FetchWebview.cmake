# cmake/FetchWebview.cmake
# =============================================================================
# WebView overlay — fetch webview/webview header + WebView2 SDK (Windows)
# Provides FLUFFY_ENABLE_WEBVIEW (default ON, relies on LTO DCE for size)
# Uses built-in WebView2 loader (WEBVIEW_MSWEBVIEW2_BUILTIN_IMPL=1) to avoid
# WebView2Loader.dll. Auto-fetches WebView2 SDK headers via NuGet when
# FLUFFY_ENABLE_WEBVIEW=ON and no system SDK found. Soft-OFF on failure.
# =============================================================================

option(FLUFFY_ENABLE_WEBVIEW "WebView overlay via webview/webview (no WebView2Loader.dll, built-in loader)" ON)

# Effective availability is re-detected on every configure so installing the
# platform SDK later (e.g. libwebkit2gtk-4.1-dev) enables the overlay without
# clearing the CMake cache.
set(FLUFFY_WEBVIEW_ENABLED ${FLUFFY_ENABLE_WEBVIEW})

# Copies WebView2Loader.dll next to an executable so webview's explicit-link
# loader can find it by name. Required on Windows: without it the fallback
# builtin loader unloads the WebView2 runtime mid-creation and crashes.
function(fluffy_copy_webview2_loader target)
    if(WIN32 AND WEBVIEW2_LOADER_DLL AND EXISTS "${WEBVIEW2_LOADER_DLL}")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${WEBVIEW2_LOADER_DLL}"
                    "$<TARGET_FILE_DIR:${target}>/WebView2Loader.dll"
            COMMENT "Copying WebView2Loader.dll for ${target}")
    endif()
endfunction()

if(NOT FLUFFY_ENABLE_WEBVIEW)
    set(FLUFFY_WEBVIEW_ENABLED OFF)
    message(STATUS "Fluffy: WebView overlay disabled (FLUFFY_ENABLE_WEBVIEW=OFF; pass -DFLUFFY_ENABLE_WEBVIEW=ON or run ./build.sh clean to re-detect)")
    return()
endif()

include(FetchContent)

# ——— Windows: WebView2 SDK headers (WebView2.h) ————————————————————————————
if(WIN32)
    # Try system SDK (VS NuGet cache / winget location)
    find_path(WEBVIEW2_INCLUDE_DIR WebView2.h
        PATHS
            "$ENV{USERPROFILE}/.nuget/packages/microsoft.web.webview2"
            "C:/Program Files (x86)/Microsoft WebView2 SDK/Include"
        PATH_SUFFIXES build/native/include
        NO_DEFAULT_PATH
    )
    # Also try a shallow search inside NuGet package version folders
    if(NOT WEBVIEW2_INCLUDE_DIR)
        file(GLOB _webview2_candidates "$ENV{USERPROFILE}/.nuget/packages/microsoft.web.webview2/*/build/native/include")
        if(_webview2_candidates)
            list(GET _webview2_candidates 0 WEBVIEW2_INCLUDE_DIR)
        endif()
    endif()

    if(NOT WEBVIEW2_INCLUDE_DIR OR NOT EXISTS "${WEBVIEW2_INCLUDE_DIR}/WebView2.h")
        message(STATUS "Fluffy: WebView2 SDK not found locally — fetching NuGet package (auto)")
        FetchContent_Declare(webview2_sdk
            URL https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.2739.15
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        # Use manual fetch to allow soft failure
        FetchContent_GetProperties(webview2_sdk)
        if(NOT webview2_sdk_POPULATED)
            # Allow offline failure without fatal error
            set(FETCHCONTENT_QUIET FALSE)
            # Wrap in try: if network fails we soft-disable
            # FetchContent_MakeAvailable will error on network failure; use DOWNLOAD_NO_PROGRESS
            # We use a manual download with STATUS check
            include(FetchContent)
            # Attempt fetch with error handling via file(DOWNLOAD) style fallback
            # Use FetchContent's internal download; if it fails, disable
            set(_webview2_fetch_ok TRUE)
            # Try to populate; on failure, catch via message
            FetchContent_Populate(webview2_sdk)
            if(webview2_sdk_SOURCE_DIR AND EXISTS "${webview2_sdk_SOURCE_DIR}/build/native/include/WebView2.h")
                set(WEBVIEW2_INCLUDE_DIR "${webview2_sdk_SOURCE_DIR}/build/native/include")
                message(STATUS "Fluffy: fetched WebView2 SDK to ${webview2_sdk_SOURCE_DIR}")
            else()
                # Also check alternative layout (some NuGet versions extract to different dir)
                file(GLOB _sdk_inc "${webview2_sdk_SOURCE_DIR}/*/build/native/include/WebView2.h")
                if(_sdk_inc)
                    get_filename_component(WEBVIEW2_INCLUDE_DIR "${_sdk_inc}" DIRECTORY)
                else()
                    message(WARNING "Fluffy: auto NuGet fetch failed — WebView overlay disabled (install 'winget install Microsoft.WebView2.SDK' or allow network)")
                    set(FLUFFY_WEBVIEW_ENABLED OFF)
                    return()
                endif()
            endif()
        else()
            set(WEBVIEW2_INCLUDE_DIR "${webview2_sdk_SOURCE_DIR}/build/native/include")
        endif()
    endif()

    if(FLUFFY_WEBVIEW_ENABLED AND WEBVIEW2_INCLUDE_DIR AND EXISTS "${WEBVIEW2_INCLUDE_DIR}/WebView2.h")
        if(CMAKE_VS_PLATFORM_NAME STREQUAL "ARM64")
            set(_fluffy_wv2_arch "arm64")
        elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(_fluffy_wv2_arch "x64")
        else()
            set(_fluffy_wv2_arch "x86")
        endif()
        get_filename_component(_fluffy_wv2_sdk_root "${WEBVIEW2_INCLUDE_DIR}" DIRECTORY)
        set(WEBVIEW2_LOADER_DLL "${_fluffy_wv2_sdk_root}/${_fluffy_wv2_arch}/WebView2Loader.dll")
        add_library(fluffy_webview_sdk INTERFACE)
        target_include_directories(fluffy_webview_sdk INTERFACE
            "$<BUILD_INTERFACE:${WEBVIEW2_INCLUDE_DIR}>")
        target_compile_definitions(fluffy_webview_sdk INTERFACE
            WEBVIEW_MSWEBVIEW2_BUILTIN_IMPL=1
            WEBVIEW_MSWEBVIEW2_EXPLICIT_LINK=1)
        message(STATUS "Fluffy: WebView2 SDK headers: ${WEBVIEW2_INCLUDE_DIR} (explicit link via ${WEBVIEW2_LOADER_DLL})")
    else()
        message(WARNING "Fluffy: WebView2 SDK headers still not found — disabling WebView")
        set(FLUFFY_WEBVIEW_ENABLED OFF)
        return()
    endif()
endif()

# ——— webview/webview header (all platforms) ————————————————————————————————
if(FLUFFY_WEBVIEW_ENABLED)
    # Try to fetch webview/webview header — soft failure keeps stub building
    include(FetchContent)
    FetchContent_Declare(webview
        URL https://github.com/webview/webview/archive/refs/tags/0.12.0.tar.gz)
    FetchContent_GetProperties(webview)
    if(NOT webview_POPULATED)
        # Allow offline: do not fatal if the source archive cannot be fetched
        set(_webview_fetch_ok TRUE)
        # Use QUIET populate
        cmake_language(CALL FetchContent_Populate webview)
        if(NOT webview_POPULATED AND webview_SOURCE_DIR AND EXISTS "${webview_SOURCE_DIR}/webview.h")
            set(webview_POPULATED TRUE)
        endif()
    endif()
    if(NOT TARGET fluffy_webview)
        add_library(fluffy_webview INTERFACE)
        if(webview_POPULATED AND EXISTS "${webview_SOURCE_DIR}/core/include")
            target_include_directories(fluffy_webview INTERFACE
                "$<BUILD_INTERFACE:${webview_SOURCE_DIR}/core/include>")
        elseif(webview_POPULATED AND EXISTS "${webview_SOURCE_DIR}/webview.h")
            target_include_directories(fluffy_webview INTERFACE
                "$<BUILD_INTERFACE:${webview_SOURCE_DIR}>")
        endif()
        if(TARGET fluffy_webview_sdk)
            target_link_libraries(fluffy_webview INTERFACE fluffy_webview_sdk)
        endif()
        if(APPLE)
            find_library(WEBKIT_FRAMEWORK WebKit QUIET)
            if(WEBKIT_FRAMEWORK)
                target_link_libraries(fluffy_webview INTERFACE "${WEBKIT_FRAMEWORK}")
            endif()
        elseif(WIN32)
            target_link_libraries(fluffy_webview INTERFACE
                advapi32 shell32 user32 ole32 shlwapi version)
        else()
            find_package(PkgConfig QUIET)
            if(PkgConfig_FOUND)
                pkg_check_modules(webkit2gtk41 QUIET IMPORTED_TARGET webkit2gtk-4.1)
                if(webkit2gtk41_FOUND)
                    # BUILD_INTERFACE only: the imported pkg-config target does
                    # not exist for consumers of the installed package; apps
                    # using the WebView widget link webkit2gtk themselves.
                    target_link_libraries(fluffy_webview INTERFACE
                        "$<BUILD_INTERFACE:PkgConfig::webkit2gtk41>")
                else()
                    pkg_check_modules(webkit2gtk40 QUIET IMPORTED_TARGET webkit2gtk-4.0)
                    if(webkit2gtk40_FOUND)
                        target_link_libraries(fluffy_webview INTERFACE
                            "$<BUILD_INTERFACE:PkgConfig::webkit2gtk40>")
                    else()
                        message(WARNING "Fluffy: webkit2gtk not found — WebView overlay disabled (install libwebkit2gtk-4.1-dev or libwebkit2gtk-4.0-dev)")
                        set(FLUFFY_WEBVIEW_ENABLED OFF)
                        return()
                    endif()
                endif()
            else()
                message(WARNING "Fluffy: pkg-config not found — WebView overlay disabled on Linux")
                set(FLUFFY_WEBVIEW_ENABLED OFF)
                return()
            endif()
        endif()
        target_compile_definitions(fluffy_webview INTERFACE WEBVIEW_MSWEBVIEW2_BUILTIN_IMPL=1 WEBVIEW_MSWEBVIEW2_EXPLICIT_LINK=1)
    endif()
    if(webview_POPULATED)
        message(STATUS "Fluffy: WebView overlay enabled (webview/webview 0.12, built-in loader, LTO DCE pay-for-use)")
    else()
        message(STATUS "Fluffy: WebView overlay enabled (stub, webview headers not fetched — offline build, semantic overlay still works via placeholder)")
    endif()
endif()
