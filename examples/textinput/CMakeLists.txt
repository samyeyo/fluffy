# examples/textinput/CMakeLists.txt
# ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
# textinput example: demonstration of the multiline TextInput widget.
# ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
add_executable(textinput main.cpp)

target_link_libraries(textinput
    PRIVATE fluffy
)

if(APPLE)
    set_target_properties(textinput PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/embedded_Info.plist"
    )
    add_custom_command(TARGET textinput POST_BUILD
        COMMAND strip -x $<TARGET_FILE:textinput>
        COMMENT "Stripping textinput binary (removing local symbols)"
    )
elseif(WIN32)
    set_target_properties(textinput PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
