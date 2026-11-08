add_executable(yamlui main.cpp)
target_link_libraries(yamlui PRIVATE fluffy)

if(APPLE)
    set_target_properties(yamlui PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/embedded_Info.plist"
    )
    # strip -x drops the symbols dead_strip leaves in place.
    add_custom_command(TARGET yamlui POST_BUILD
        COMMAND strip -x $<TARGET_FILE:yamlui>
        COMMENT "Stripping yamlui binary (removing local symbols)"
    )
elseif(WIN32)
    set_target_properties(yamlui PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_options(yamlui PRIVATE "/ENTRY:mainCRTStartup")
endif()

# So win.load("ui.yaml") resolves when run from the command line.
add_custom_command(TARGET yamlui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/ui.yaml"
            "$<TARGET_FILE_DIR:yamlui>/ui.yaml"
    COMMENT "Mirroring ui.yaml next to binary"
    VERBATIM)
