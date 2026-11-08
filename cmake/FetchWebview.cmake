option(FLUFFY_ENABLE_WEBVIEW "WebView overlay via webview/webview (no WebView2Loader.dll, built-in loader)" ON)

set(FLUFFY_WEBVIEW_ENABLED ${FLUFFY_ENABLE_WEBVIEW})

if(NOT FLUFFY_ENABLE_WEBVIEW)
    set(FLUFFY_WEBVIEW_ENABLED OFF)
    message(STATUS "Fluffy: WebView overlay disabled (FLUFFY_ENABLE_WEBVIEW=OFF; pass -DFLUFFY_ENABLE_WEBVIEW=ON or run ./build.sh clean to re-detect)")
    return()
endif()

include(FetchContent)

if(WIN32)
    find_path(WEBVIEW2_INCLUDE_DIR WebView2.h
        PATHS
            "$ENV{USERPROFILE}/.nuget/packages/microsoft.web.webview2"
            "C:/Program Files (x86)/Microsoft WebView2 SDK/Include"
        PATH_SUFFIXES build/native/include
        NO_DEFAULT_PATH
    )
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
        FetchContent_GetProperties(webview2_sdk)
        if(NOT webview2_sdk_POPULATED)
            set(FETCHCONTENT_QUIET FALSE)
            # Populate rather than MakeAvailable: a network failure should disable, not abort.
            FetchContent_Populate(webview2_sdk)
            if(webview2_sdk_SOURCE_DIR AND EXISTS "${webview2_sdk_SOURCE_DIR}/build/native/include/WebView2.h")
                set(WEBVIEW2_INCLUDE_DIR "${webview2_sdk_SOURCE_DIR}/build/native/include")
                message(STATUS "Fluffy: fetched WebView2 SDK to ${webview2_sdk_SOURCE_DIR}")
            else()
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
        add_library(fluffy_webview_sdk INTERFACE)
        target_include_directories(fluffy_webview_sdk INTERFACE
            "$<BUILD_INTERFACE:${WEBVIEW2_INCLUDE_DIR}>")
        target_compile_definitions(fluffy_webview_sdk INTERFACE
            WEBVIEW_MSWEBVIEW2_BUILTIN_IMPL=1
            WEBVIEW_MSWEBVIEW2_EXPLICIT_LINK=1)
        message(STATUS "Fluffy: WebView2 SDK headers: ${WEBVIEW2_INCLUDE_DIR}")
    else()
        message(WARNING "Fluffy: WebView2 SDK headers still not found — disabling WebView")
        set(FLUFFY_WEBVIEW_ENABLED OFF)
        return()
    endif()
endif()

if(FLUFFY_WEBVIEW_ENABLED)
    include(FetchContent)
    FetchContent_Declare(webview
        URL https://github.com/webview/webview/archive/refs/tags/0.12.0.tar.gz)
    FetchContent_GetProperties(webview)
    if(NOT webview_POPULATED)
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
                    # BUILD_INTERFACE: the pkg-config target does not exist for installed-package consumers.
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
