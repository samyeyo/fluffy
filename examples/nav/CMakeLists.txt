add_executable(nav main.cpp)

target_link_libraries(nav
    PRIVATE fluffy
)

if(APPLE)
    set_target_properties(nav PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/embedded_Info.plist"
    )
    add_custom_command(TARGET nav POST_BUILD
        COMMAND strip -x $<TARGET_FILE:nav>
        COMMENT "Stripping nav binary"
    )
elseif(WIN32)
    set_target_properties(nav PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_options(nav PRIVATE "/ENTRY:mainCRTStartup")
endif()
