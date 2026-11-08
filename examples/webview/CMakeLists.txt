add_executable(webview main.cpp)

target_link_libraries(webview
    PRIVATE fluffy
)

if(APPLE)
    set_target_properties(webview PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/embedded_Info.plist"
    )
    add_custom_command(TARGET webview POST_BUILD
        COMMAND strip -x $<TARGET_FILE:webview>
        COMMENT "Stripping webview binary (removing local symbols)"
    )
elseif(WIN32)
    set_target_properties(webview PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_options(webview PRIVATE "/ENTRY:mainCRTStartup")
endif()
