add_executable(multiwindow main.cpp)

target_link_libraries(multiwindow
    PRIVATE fluffy
)

if(APPLE)
    set_target_properties(multiwindow PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/embedded_Info.plist"
    )
    add_custom_command(TARGET multiwindow POST_BUILD
        COMMAND strip -x $<TARGET_FILE:multiwindow>
        COMMENT "Stripping multiwindow binary"
    )
elseif(WIN32)
    set_target_properties(multiwindow PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_options(multiwindow PRIVATE "/ENTRY:mainCRTStartup")
endif()
