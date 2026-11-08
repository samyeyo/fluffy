add_executable(icons main.cpp)

target_link_libraries(icons PRIVATE fluffy)

if(APPLE)
    set_target_properties(icons PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/embedded_Info.plist"
    )
    # strip -x drops the symbols dead_strip leaves in place.
    add_custom_command(TARGET icons POST_BUILD
        COMMAND strip -x $<TARGET_FILE:icons>
        COMMENT "Stripping icons binary (removing local symbols)"
    )
elseif(WIN32)
    set_target_properties(icons PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_options(icons PRIVATE "/ENTRY:mainCRTStartup")
endif()
