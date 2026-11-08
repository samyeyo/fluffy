# examples/helloworld/CMakeLists.txt
# ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
# Hello World example: minimal demonstration of Fluffy.
# ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
add_executable(helloworld main.cpp)

target_link_libraries(helloworld
    PRIVATE fluffy
)

if(APPLE)
    set_target_properties(helloworld PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/embedded_Info.plist"
    )
    # Strip local symbols from the final executable — with LTO + dead_strip
    # already removing unused code, this discards the remaining symbol
    # table entries that are not needed for execution.
    add_custom_command(TARGET helloworld POST_BUILD
        COMMAND strip -x $<TARGET_FILE:helloworld>
        COMMENT "Stripping helloworld binary (removing local symbols)"
    )
elseif(WIN32)
    set_target_properties(helloworld PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_options(helloworld PRIVATE "/ENTRY:mainCRTStartup")
endif()
