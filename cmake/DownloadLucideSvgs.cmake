# cmake/DownloadLucideSvgs.cmake
# =============================================================================
# Download individual Lucide SVG icons and generate a C++ source file that
# contains each icon's SVG XML as a separate `extern const char kXxxSvg[]`
# variable.  With `-fdata-sections` + `-Wl,-dead_strip`, the linker
# discards SVGs for icons the app never references.
#
# Icon naming convention:
#   Icon names use underscores (e.g. door_open) -> door-open.svg  (underscore -> hyphen)
# =============================================================================

# List of icons to download.  Each entry is the underscore-separated name
# We download ALL icons used in the project.
set(_fluffy_svg_icons
    door_open rocket check heart star zap flame
    bell calendar camera clock_4 cog compass
    archive award bug clipboard cloud
    battery book box briefcase building car
    arrow_down arrow_left arrow_right arrow_up chevron_down
    activity anchor bluetooth bot circle
    warehouse wifi wrench x
    # -- batch 2: commonly used UI icons --
    airplay alarm_clock alert_triangle at_sign badge_check ban
    battery_charging battery_full bookmark calculator cast
    check_circle chevron_left chevron_right chevron_up
    clipboard_copy clock cloud_download cloud_upload coffee
    copy cpu credit_card database download edit
    eye eye_off file_text flag folder folder_open gift
    globe headphones home image inbox key layers
    lightbulb link loader lock log_in log_out mail
    map_pin maximize menu mic minimize monitor moon
    music pause pencil phone pin play power refresh
    save scan search send shield shopping_cart smartphone
    smile sun tag target text trash unlock upload
    user users video wallet zoom_in zoom_out
    # -- batch 3: more UI/media/communication icons --
    accessibility album ambulance apple archive_restore
    atom axe baby backpack balloon barcode
    bike binoculars bird bitcoin bold bone
    book_open bookmark_check bookmark_plus bottle_wine brain
    brick_wall brush building_2 bus cake candle
    car_front cat chart_bar chart_pie check_check
    chevrons_down chevrons_left chevrons_right chevrons_up
    circle_plus circle_slash clapperboard clipboard_check clipboard_list
    cloud_drizzle cloud_fog cloud_lightning cloud_snow clover
    club coins command component computer cone
    construction contact contrast cookie copyright
    crosshair crown cube dice dna dock
    dog donut ear earth eclipse egg
    eraser euro expand external_link ferris_wheel figma
    file_archive file_audio file_check file_code file_diff
    file_image file_json file_key file_lock file_minus file_plus
    # -- batch 4: align, editing, media, coding --
    align_center align_justify align_left align_right
    antenna anvil aperture audio_lines audio_waveform
    banana bandage banknote barrel bath
    beaker bed bell_off bell_plus bell_ring
    binary biohazard blinds blocks bolt
    book_alert book_check book_heart book_plus book_text
    bot_off boxes braces brackets bring_to_front
    butterfly cactus candy_off cassette_tape cherry
    chevron_first chevron_last chevrons_up_down
    circle_dot circle_fading_plus circle_pause circle_play
    circle_stop columns_4 conveyor_belt creative_commons
    crop diff divide dots_horizontal dots_vertical
    drill droplet excavator eyebrow factory
    farm fence filter fingerprint fire_extinguisher
    fish flag_off flag_triangle flask_conical flask_round
    flip_horizontal flip_vertical flower flower_2 focus
    folder_archive folder_cog folder_heart folder_input
    folder_key folder_lock folder_minus folder_open_2
    folder_output folder_plus folder_search folder_symlink
    folder_tree folder_up folder_x font folder_clock
    # -- batch 5: all remaining Lucide icons --
    a_arrow_down a_arrow_up a_large_small ad air_vent alarm_clock_check alarm_clock_minus alarm_clock_off alarm_clock_plus alarm_smoke align_center_horizontal align_center_vertical
    align_end_horizontal align_end_vertical align_horizontal_distribute_center align_horizontal_distribute_end align_horizontal_distribute_start align_horizontal_justify_center align_horizontal_justify_end align_horizontal_justify_start align_horizontal_space_around align_horizontal_space_between align_start_horizontal align_start_vertical
    align_vertical_distribute_center align_vertical_distribute_end align_vertical_distribute_start align_vertical_justify_center align_vertical_justify_end align_vertical_justify_start align_vertical_space_around align_vertical_space_between ampersand ampersands amphora angry
    annoyed app_window app_window_mac archive_x armchair arrow_big_down arrow_big_down_dash arrow_big_left arrow_big_left_dash arrow_big_right arrow_big_right_dash arrow_big_up
    arrow_big_up_dash arrow_down_0_1 arrow_down_1_0 arrow_down_a_z arrow_down_from_line arrow_down_left arrow_down_narrow_wide arrow_down_right arrow_down_to_dot arrow_down_to_line arrow_down_up arrow_down_wide_narrow
    arrow_down_z_a arrow_left_from_line arrow_left_right arrow_left_to_line arrow_right_from_line arrow_right_left arrow_right_to_line arrow_up_0_1 arrow_up_1_0 arrow_up_a_z arrow_up_down arrow_up_from_dot
    arrow_up_from_line arrow_up_left arrow_up_narrow_wide arrow_up_right arrow_up_to_line arrow_up_wide_narrow arrow_up_z_a arrows_up_from_line asterisk astroid axis_3d badge
    badge_alert badge_cent badge_dollar_sign badge_euro badge_indian_rupee badge_info badge_japanese_yen badge_minus badge_percent badge_plus badge_pound_sterling badge_question_mark
    badge_russian_ruble badge_swiss_franc badge_turkish_lira badge_x baggage_claim banknote_arrow_down banknote_arrow_up banknote_check banknote_x baseline battery_low battery_medium
    battery_plus battery_warning bean bean_off bed_double bed_single beef beef_off beer beer_off bell_check bell_dot
    bell_electric bell_minus between_horizontal_end between_horizontal_start between_vertical_end between_vertical_start biceps_flexed birdhouse blend blender bluetooth_connected bluetooth_off
    bluetooth_searching bomb bone_fracture book_a book_audio book_copy book_dashed book_down book_headphones book_image book_key book_lock
    book_marked book_minus book_open_check book_open_text book_search book_type book_up book_up_2 book_user book_x bookmark_minus bookmark_off
    bookmark_x boom_box bot_message_square bow_arrow brain_circuit brain_cog brick_wall_fire brick_wall_shield briefcase_business briefcase_conveyor_belt briefcase_medical broccoli
    brush_cleaning bubbles bug_off bug_play bus_front cable cable_car cake_slice calendar_1 calendar_arrow_down calendar_arrow_up calendar_check
    calendar_check_2 calendar_clock calendar_cog calendar_days calendar_fold calendar_heart calendar_minus calendar_minus_2 calendar_off calendar_plus calendar_plus_2 calendar_range
    calendar_search calendar_sync calendar_x calendar_x_2 calendars camera_off candy candy_cane cannabis cannabis_off captions captions_off
    car_taxi_front caravan card_sim carrot case_lower case_sensitive case_upper castle cctv cctv_off chart_area chart_bar_big
    chart_bar_decreasing chart_bar_increasing chart_bar_stacked chart_candlestick chart_column chart_column_big chart_column_decreasing chart_column_increasing chart_column_stacked chart_gantt chart_line chart_network
    chart_no_axes_column chart_no_axes_column_decreasing chart_no_axes_column_increasing chart_no_axes_combined chart_no_axes_gantt chart_scatter chart_spline check_line chef_hat chess_bishop chess_king chess_knight
    chess_pawn chess_queen chess_rook chevrons_down_up chevrons_left_right chevrons_left_right_ellipsis chevrons_right_left church cigarette cigarette_off circle_alert circle_arrow_down
    circle_arrow_left circle_arrow_out_down_left circle_arrow_out_down_right circle_arrow_out_up_left circle_arrow_out_up_right circle_arrow_right circle_arrow_up circle_check circle_check_big circle_chevron_down circle_chevron_left circle_chevron_right
    circle_chevron_up circle_dashed circle_divide circle_dollar_sign circle_dot_dashed circle_ellipsis circle_equal circle_euro circle_fading_arrow_up circle_gauge circle_minus circle_off
    circle_parking circle_parking_off circle_percent circle_pile circle_pound_sterling circle_power circle_question_mark circle_slash_2 circle_small circle_star circle_user circle_user_round
    circle_x circuit_board citrus clipboard_clock clipboard_minus clipboard_paste clipboard_pen clipboard_pen_line clipboard_plus clipboard_type clipboard_x clock_1
    clock_10 clock_11 clock_12 clock_2 clock_3 clock_5 clock_6 clock_7 clock_8 clock_9 clock_alert clock_arrow_down
    clock_arrow_left clock_arrow_right clock_arrow_up clock_check clock_fading clock_plus closed_caption cloud_alert cloud_backup cloud_check cloud_cog cloud_hail
    cloud_moon cloud_moon_rain cloud_off cloud_rain cloud_rain_wind cloud_sun cloud_sun_rain cloud_sync cloudy code code_xml columns_2
    columns_3 columns_3_cog combine concierge_bell contact_round container cooking_pot copy_check copy_minus copy_plus copy_slash copy_x
    copyleft corner_down_left corner_down_right corner_left_down corner_left_up corner_right_down
)

# Output directories
set(_fluffy_svg_dir "${CMAKE_CURRENT_BINARY_DIR}/third_party/lucide_svg")
set(_fluffy_svg_src "${CMAKE_CURRENT_BINARY_DIR}/generated/src/lucide_svg_data.cpp")
file(MAKE_DIRECTORY "${_fluffy_svg_dir}")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/src")

# Initialize the generated source file
file(WRITE "${_fluffy_svg_src}"
    "// Generated by cmake/DownloadLucideSvgs.cmake -- do not edit\n"
    "#include <cstddef>\n"
    "#include <cstdint>\n"
    "#include <fluffy/svg_icon.hpp>\n"
    "#include <string_view>\n"
    "namespace fluffy { namespace Icons { namespace detail {\n"
    "\n"
)

# Lucide renamed several icons upstream, so the old file names 404.
# Map old dashed name -> new upstream dashed name. Bytes are stored
# under the old name, so the fluffy::Icons::* C++ API is unchanged.
set(_fluffy_svg_renames
    "alert-triangle:triangle-alert"
    "check-circle:circle-check-big"
    "edit:pen"
    "refresh:refresh-ccw"
)

# Track which icons downloaded successfully
set(_fluffy_downloaded_icons)

# Download each icon and append to the source
foreach(_name ${_fluffy_svg_icons})
    # door_open -> door-open
    string(REGEX REPLACE "_" "-" _svg_name "${_name}")
    set(_url "https://raw.githubusercontent.com/lucide-icons/lucide/main/icons/${_svg_name}.svg")
    set(_dest "${_fluffy_svg_dir}/${_svg_name}.svg")

    if(NOT EXISTS "${_dest}")
        file(DOWNLOAD "${_url}" "${_dest}" STATUS _dl_status)
        list(GET _dl_status 0 _dl_rc)
        if(NOT _dl_rc EQUAL 0)
            # Try the renamed upstream file before giving up.
            set(_dl_ok FALSE)
            foreach(_pair ${_fluffy_svg_renames})
                string(REPLACE ":" ";" _parts "${_pair}")
                list(GET _parts 0 _old_dash)
                list(GET _parts 1 _new_dash)
                if(_svg_name STREQUAL _old_dash)
                    set(_retry_url "https://raw.githubusercontent.com/lucide-icons/lucide/main/icons/${_new_dash}.svg")
                    file(DOWNLOAD "${_retry_url}" "${_dest}" STATUS _retry_status)
                    list(GET _retry_status 0 _retry_rc)
                    if(_retry_rc EQUAL 0)
                        set(_dl_ok TRUE)
                        message(STATUS "Downloaded Lucide SVG: ${_svg_name}.svg (as ${_new_dash}.svg)")
                    endif()
                    break()
                endif()
            endforeach()
            if(NOT _dl_ok)
                # Remove the (possibly empty) file left behind so a later
                # re-configure retries the download instead of baking in
                # an empty icon or dropping the symbol entirely.
                file(REMOVE "${_dest}")
                message(WARNING "Failed to download ${_url} — skipping")
                continue()
            endif()
        else()
            message(STATUS "Downloaded Lucide SVG: ${_svg_name}.svg")
        endif()
    endif()

    file(READ "${_dest}" _svg_content)
    # Compact: replace newlines with spaces, collapse runs of spaces
    string(REGEX REPLACE "\n" " " _svg_content "${_svg_content}")
    string(REGEX REPLACE "  +" " " _svg_content "${_svg_content}")
    string(STRIP "${_svg_content}" _svg_content)

    # Build symbol name: door_open -> k_door_open_Svg
    set(_sym "k_${_name}_Svg")

    # Escape for raw string literal: use a unique delimiter.
    # C++11 limits raw string delimiters to 16 characters.
    string(TOUPPER "${_name}" _delim)
    string(SUBSTRING "${_delim}" 0 16 _delim)
    set(_raw "R\"${_delim}(${_svg_content})${_delim}\"")

    string(LENGTH "${_svg_content}" _svg_byte_len)
    file(APPEND "${_fluffy_svg_src}"
        "extern const char ${_sym}[] = ${_raw};\n"
        "extern const std::size_t ${_sym}Size = ${_svg_byte_len};\n"
        "\n"
    )

    list(APPEND _fluffy_downloaded_icons "${_name}")
endforeach()

file(APPEND "${_fluffy_svg_src}"
    "}}}  // namespace fluffy::Icons::detail\n"
    "\n"
    "// SvgIconRef constants for each icon — one per icon.\n"
    "namespace fluffy { namespace Icons {\n"
    "\n"
)

foreach(_name ${_fluffy_downloaded_icons})
    file(APPEND "${_fluffy_svg_src}"
        "extern const SvgIconRef ${_name} = {\n"
        "    detail::k_${_name}_Svg,\n"
        "    detail::k_${_name}_SvgSize\n"
        "};\n"
        "\n"
    )
endforeach()

file(APPEND "${_fluffy_svg_src}"
    "const SvgIconRef* find(std::string_view name) noexcept {\n"
    "    struct Entry { std::string_view name; const SvgIconRef* icon; };\n"
    "    static const Entry entries[] = {\n"
)

foreach(_name ${_fluffy_downloaded_icons})
    file(APPEND "${_fluffy_svg_src}"
        "        {\"${_name}\", &${_name}},\n"
    )
endforeach()

file(APPEND "${_fluffy_svg_src}"
    "    };\n"
    "    for (const auto& entry : entries) {\n"
    "        if (entry.name == name) return entry.icon;\n"
    "    }\n"
    "    return nullptr;\n"
    "}\n"
    "}}  // namespace fluffy::Icons\n"
)

# Expose the generated source path to the parent CMakeLists.txt.
# include() runs in the same scope, so PARENT_SCOPE is NOT needed.
list(LENGTH _fluffy_downloaded_icons _fluffy_icon_count)
set(FLUFFY_LUCIDE_SVG_DATA "${_fluffy_svg_src}")
message(STATUS "Generated Lucide SVG data (${_fluffy_icon_count} icons) -> ${_fluffy_svg_src}")
