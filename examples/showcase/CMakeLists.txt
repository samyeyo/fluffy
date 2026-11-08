# examples/showcase/CMakeLists.txt
# ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
# Showcase example: all widgets in their natural size via autoSize + layout.
# ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
add_executable(showcase main.cpp)

target_link_libraries(showcase
    PRIVATE fluffy
)

if(APPLE)
    set_target_properties(showcase PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/embedded_Info.plist"
    )
    add_custom_command(TARGET showcase POST_BUILD
        COMMAND strip -x $<TARGET_FILE:showcase>
        COMMENT "Stripping showcase binary"
    )
elseif(WIN32)
    set_target_properties(showcase PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_options(showcase PRIVATE "/ENTRY:mainCRTStartup")
endif()

