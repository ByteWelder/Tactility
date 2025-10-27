# Buildscripts/board.cmake
# This file reads boards.json at build time

if (NOT WIN32)
    string(ASCII 27 Esc)
    set(ColorReset "${Esc}[m")
    set(Cyan "${Esc}[36m")
else ()
    set(ColorReset "")
    set(Cyan "")
endif ()

function(INIT_TACTILITY_GLOBALS SDKCONFIG_FILE)
    get_filename_component(SDKCONFIG_FILE_ABS ${SDKCONFIG_FILE} ABSOLUTE)
    
    # Find the board identifier in the sdkconfig file
    file(READ ${SDKCONFIG_FILE_ABS} sdkconfig_text)
    string(REGEX MATCH "(CONFIG_TT_BOARD_ID=\"[a-z0-9_\-]*\")" sdkconfig_board_id "${sdkconfig_text}")
    
    if (sdkconfig_board_id STREQUAL "")
        message(FATAL_ERROR "CONFIG_TT_BOARD_ID not found in sdkconfig:\nMake sure you copied one of the sdkconfig.board.* files into sdkconfig")
    endif ()
    
    string(LENGTH ${sdkconfig_board_id} sdkconfig_board_id_length)
    set(id_length 0)
    math(EXPR id_length "${sdkconfig_board_id_length} - 21")
    string(SUBSTRING ${sdkconfig_board_id} 20 ${id_length} board_id)
    message("Board name: ${Cyan}${board_id}${ColorReset}")
    
    # Read boards.json directly at build time
    set(BOARDS_JSON_PATH "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/boards.json")
    if (NOT EXISTS "${BOARDS_JSON_PATH}")
        message(FATAL_ERROR "boards.json not found at ${BOARDS_JSON_PATH}")
    endif()
    
    file(READ "${BOARDS_JSON_PATH}" BOARDS_JSON_CONTENT)
    
    # Parse JSON to find the matching board
    string(JSON BOARD_COUNT LENGTH "${BOARDS_JSON_CONTENT}")
    if (BOARD_COUNT EQUAL 0)
        message(FATAL_ERROR "No boards defined in boards.json")
    endif()
    
    math(EXPR BOARD_COUNT "${BOARD_COUNT} - 1")
    set(TACTILITY_BOARD_PROJECT "")
    
    foreach(i RANGE ${BOARD_COUNT})
        string(JSON CURRENT_BOARD_ID ERROR_VARIABLE json_error GET "${BOARDS_JSON_CONTENT}" ${i} "id")
        if (json_error)
            message(WARNING "Failed to read board ${i} id: ${json_error}")
            continue()
        endif()
        
        if(CURRENT_BOARD_ID STREQUAL "${board_id}")
            string(JSON TACTILITY_BOARD_PROJECT ERROR_VARIABLE json_error GET "${BOARDS_JSON_CONTENT}" ${i} "project")
            if (json_error)
                message(FATAL_ERROR "Board '${board_id}' found but 'project' field is missing: ${json_error}")
            endif()
            break()
        endif()
    endforeach()

    if (TACTILITY_BOARD_PROJECT STREQUAL "")
        message(FATAL_ERROR "No board with id '${board_id}' found in boards.json\nAvailable boards can be found in Buildscripts/boards.json")
    else ()
        message("Board path: ${Cyan}Boards/${TACTILITY_BOARD_PROJECT}${ColorReset}\n")
        set_property(GLOBAL PROPERTY TACTILITY_BOARD_PROJECT ${TACTILITY_BOARD_PROJECT})
        set_property(GLOBAL PROPERTY TACTILITY_BOARD_ID ${board_id})
    endif ()
endfunction()
