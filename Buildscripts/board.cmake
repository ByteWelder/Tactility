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
    
    # Read boards.json directly at build time
    set(BOARDS_JSON_PATH "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/boards.json")
    if (NOT EXISTS "${BOARDS_JSON_PATH}")
        message(FATAL_ERROR "boards.json not found at ${BOARDS_JSON_PATH}")
    endif()
    
    file(READ "${BOARDS_JSON_PATH}" BOARDS_JSON_CONTENT)
 
    # Parse JSON to find the matching board
    string(JSON ROOT_TYPE TYPE "${BOARDS_JSON_CONTENT}")
    if (NOT ROOT_TYPE STREQUAL "ARRAY")
        message(FATAL_ERROR "boards.json root must be a JSON array, got: ${ROOT_TYPE}")
    endif()
    string(JSON BOARD_COUNT LENGTH "${BOARDS_JSON_CONTENT}")
    if (BOARD_COUNT EQUAL 0)
        message(FATAL_ERROR "No boards defined in boards.json")
    endif()
    
    math(EXPR BOARD_COUNT "${BOARD_COUNT} - 1")
    set(TACTILITY_BOARD_PROJECT "")
    set(AVAILABLE_BOARD_IDS "")
    
    foreach(i RANGE ${BOARD_COUNT})
        string(JSON CURRENT_BOARD_ID ERROR_VARIABLE json_error GET "${BOARDS_JSON_CONTENT}" ${i} "id")
        if (json_error)
            message(WARNING "Failed to read board ${i} id: ${json_error}")
            continue()
        endif()
        list(APPEND AVAILABLE_BOARD_IDS "${CURRENT_BOARD_ID}")
        
        if(CURRENT_BOARD_ID STREQUAL "${board_id}")
            string(JSON TACTILITY_BOARD_PROJECT ERROR_VARIABLE json_error GET "${BOARDS_JSON_CONTENT}" ${i} "project")
            if (json_error)
                message(FATAL_ERROR "Board '${board_id}' found but 'project' field is missing: ${json_error}")
            endif()
            break()
        endif()
    endforeach()

    if (TACTILITY_BOARD_PROJECT STREQUAL "")
        list(REMOVE_DUPLICATES AVAILABLE_BOARD_IDS)
        list(JOIN AVAILABLE_BOARD_IDS ", " AVAILABLE_BOARD_IDS_JOINED)
        message(FATAL_ERROR "No board with id '${board_id}' found in boards.json.\n"
                            "Available ids: [${AVAILABLE_BOARD_IDS_JOINED}]\n"
                            "Ensure you have the correct boards.json file in ${CMAKE_CURRENT_FUNCTION_LIST_DIR}")
    else ()
        message("Board path: ${Cyan}Boards/${TACTILITY_BOARD_PROJECT}${ColorReset}\n")
        set_property(GLOBAL PROPERTY TACTILITY_BOARD_PROJECT "${TACTILITY_BOARD_PROJECT}")
        set_property(GLOBAL PROPERTY TACTILITY_BOARD_ID "${board_id}")
    endif ()
endfunction()
