# Buildscripts/board.cmake
# This file reads boards.json at build time

if (NOT WIN32)
    if (NOT DEFINED ENV{CI})
        string(ASCII 27 Esc)
        set(ColorReset "${Esc}[m")
        set(Cyan "${Esc}[36m")
    else()
        set(ColorReset "")
        set(Cyan "")
    endif()
else ()
    set(ColorReset "")
    set(Cyan "")
endif ()

function(INIT_TACTILITY_GLOBALS SDKCONFIG_FILE)
    get_filename_component(SDKCONFIG_FILE_ABS ${SDKCONFIG_FILE} ABSOLUTE)

    # Validate sdkconfig file existence
    if (NOT EXISTS "${SDKCONFIG_FILE_ABS}")
        message(FATAL_ERROR "sdkconfig file not found: ${SDKCONFIG_FILE_ABS}")
    endif()
    
    # Find the board identifier in the sdkconfig file
    file(READ "${SDKCONFIG_FILE_ABS}" sdkconfig_text)
    string(REGEX MATCH "CONFIG_TT_BOARD_ID=\"([^\"]*)\"" sdkconfig_board_id "${sdkconfig_text}")
    
    if (sdkconfig_board_id STREQUAL "")
        message(FATAL_ERROR "CONFIG_TT_BOARD_ID not found in sdkconfig:\nMake sure you copied one of the sdkconfig.board.* files into sdkconfig")
    endif ()
    
    # Extract captured group
    string(REGEX REPLACE ".*CONFIG_TT_BOARD_ID=\"([^\"]*)\".*" "\\1" board_id "${sdkconfig_text}")
    message("Board name: ${Cyan}${board_id}${ColorReset}")
    set(TACTILITY_BOARD_PROJECT ${board_id})
    message("Board path: ${Cyan}Boards/${TACTILITY_BOARD_PROJECT}${ColorReset}\n")
    set_property(GLOBAL PROPERTY TACTILITY_BOARD_PROJECT ${TACTILITY_BOARD_PROJECT})
    set_property(GLOBAL PROPERTY TACTILITY_BOARD_ID ${board_id})
endfunction()
