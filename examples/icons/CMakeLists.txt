# examples/icons/CMakeLists.txt
# ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
# Icons example — visual catalogue of Lucide SVG icons.
# ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
add_executable(icons main.cpp)

target_link_libraries(icons PRIVATE fluffy)

if(APPLE)
    set_target_properties(icons PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/embedded_Info.plist"
    )
    # Strip local symbols — with LTO + dead_strip already removing unused
    # code, this discards the remaining symbol table entries not needed
    # for execution, dramatically reducing binary size.
    add_custom_command(TARGET icons POST_BUILD
        COMMAND strip -x $<TARGET_FILE:icons>
        COMMENT "Stripping icons binary (removing local symbols)"
    )
elseif(WIN32)
    set_target_properties(icons PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_options(icons PRIVATE "/ENTRY:mainCRTStartup")
endif()
