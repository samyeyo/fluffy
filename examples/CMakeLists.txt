# Documentation image generators are intentionally not CMake targets.
# www/gendocs.py owns the checked-in website assets and syncs them into www/.
set(_fluffy_examples
    helloworld
    multiwindow
    webview
    icons
    textinput
    yamlui
    showcase
    menus
    nav
)

foreach(_example IN LISTS _fluffy_examples)
    add_subdirectory(${_example})
endforeach()

if(CMAKE_CONFIGURATION_TYPES)
    foreach(_example IN LISTS _fluffy_examples)
        foreach(_config IN LISTS CMAKE_CONFIGURATION_TYPES)
            string(TOUPPER "${_config}" _config_upper)
            set_target_properties(${_example} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY_${_config_upper} "${CMAKE_CURRENT_BINARY_DIR}/${_example}")
        endforeach()
    endforeach()
endif()
