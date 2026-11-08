# examples/menus/CMakeLists.txt
# ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
# Menus example: horizontal and vertical Menu widget demonstration.
# ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
add_executable(menus main.cpp)

target_link_libraries(menus
    PRIVATE fluffy
)

if(APPLE)
    set_target_properties(menus PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/embedded_Info.plist"
    )
    add_custom_command(TARGET menus POST_BUILD
        COMMAND strip -x $<TARGET_FILE:menus>
        COMMENT "Stripping menus binary"
    )
elseif(WIN32)
    set_target_properties(menus PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_options(menus PRIVATE "/ENTRY:mainCRTStartup")
endif()
