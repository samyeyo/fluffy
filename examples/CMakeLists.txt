# Documentation image generators are intentionally not CMake targets.
# www/gendocs.py owns the checked-in website assets and syncs them into www/.
add_subdirectory(helloworld)
add_subdirectory(multiwindow)

add_subdirectory(webview)

add_subdirectory(icons)

add_subdirectory(textinput)

add_subdirectory(yamlui)

add_subdirectory(showcase)

add_subdirectory(menus)

add_subdirectory(nav)
