function(INIT_TACTILITY_GLOBALS SDKCONFIG_FILE)
    # Find the board identifier in the sdkconfig file
    file(READ ${SDKCONFIG_FILE} sdkconfig_text)
    string(REGEX MATCH "(CONFIG_TT_BOARD_ID\=\"[a-z0-9_\-]*\")" sdkconfig_board_id "${sdkconfig_text}")
    if (sdkconfig_board_id STREQUAL "")
        message(FATAL_ERROR "CONFIG_TT_BOARD_ID not found in sdkconfig:\nMake sure you copied one of the sdkconfig.board.* files into sdkconfig")
    endif ()
    string(LENGTH ${sdkconfig_board_id} sdkconfig_board_id_length)
    set(id_length 0)
    math(EXPR id_length "${sdkconfig_board_id_length} - 21")
    string(SUBSTRING ${sdkconfig_board_id} 20 ${id_length} board_id)
    message("Building board ${board_id}")

    if (board_id STREQUAL "cyd-2432s024c")
        set(TACTILITY_BOARD_PROJECT CYD-2432S024C)
    elseif (board_id STREQUAL "cyd-2432s032c")
        set(TACTILITY_BOARD_PROJECT CYD-2432S032C)
    elseif (board_id STREQUAL "cyd-4848s040c")
        set(TACTILITY_BOARD_PROJECT CYD-4848S040C)
    elseif (board_id STREQUAL "cyd-8048S043c")
        set(TACTILITY_BOARD_PROJECT CYD-8048S043C)
    elseif (board_id STREQUAL "cyd-jc2432w328c")
        set(TACTILITY_BOARD_PROJECT CYD-JC2432W328C)
    elseif (board_id STREQUAL "cyd-jc8048w550c")
        set(TACTILITY_BOARD_PROJECT CYD-JC8048W550C)
    elseif (board_id STREQUAL "elecrow-crowpanel-advance-28")
        set(TACTILITY_BOARD_PROJECT ElecrowCrowpanelAdvance28)
    elseif (board_id STREQUAL "elecrow-crowpanel-advance-35")
        set(TACTILITY_BOARD_PROJECT ElecrowCrowPanelAdvance35)
    elseif (board_id STREQUAL "elecrow-crowpanel-advance-50")
        set(TACTILITY_BOARD_PROJECT ElecrowCrowPanelAdvance50)
    elseif (board_id STREQUAL "elecrow-crowpanel-basic-28")
        set(TACTILITY_BOARD_PROJECT ElecrowCrowPanelBasic28)
    elseif (board_id STREQUAL "elecrow-crowpanel-basic-35")
        set(TACTILITY_BOARD_PROJECT ElecrowCrowPanelBasic35)
    elseif (board_id STREQUAL "elecrow-crowpanel-basic-50")
        set(TACTILITY_BOARD_PROJECT ElecrowCrowPanelBasic50)
    elseif (board_id STREQUAL "lilygo-tdeck")
        set(TACTILITY_BOARD_PROJECT LilygoTdeck)
    elseif (board_id STREQUAL "m5stack-core2")
        set(TACTILITY_BOARD_PROJECT M5stackCore2)
    elseif (board_id STREQUAL "m5stack-cores3")
        set(TACTILITY_BOARD_PROJECT M5stackCoreS3)
    elseif (board_id STREQUAL "unphone")
        set(TACTILITY_BOARD_PROJECT UnPhone)
    elseif (board_id STREQUAL "waveshare-s3-touch-43")
        set(TACTILITY_BOARD_PROJECT WaveshareS3Touch43)
    else ()
        set(TACTILITY_BOARD_PROJECT "")
    endif ()

    if (TACTILITY_BOARD_PROJECT STREQUAL "")
        message(FATAL_ERROR "No subproject mapped to \"${TACTILITY_BOARD_ID}\" in root Buildscripts/board.cmake")
    else ()
        message("Board project: Boards/${TACTILITY_BOARD_PROJECT}")
        set_property(GLOBAL PROPERTY TACTILITY_BOARD_PROJECT ${TACTILITY_BOARD_PROJECT})
        set_property(GLOBAL PROPERTY TACTILITY_BOARD_ID ${board_id})
    endif ()
endfunction()
