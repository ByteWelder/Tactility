cmake_minimum_required(VERSION 3.16)

add_definitions(-DTT_DEBUG)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

if (IDF_TARGET)
    set(COMPONENTS main tactility tactility-data)
    set(EXTRA_COMPONENT_DIRS "boards" "components")
    add_definitions(-DESP_TARGET)
    
    # Yellow Board only runs on ESP32
    if(NOT "${IDF_TARGET}" STREQUAL "esp32")
        set(EXCLUDE_COMPONENTS "yellow_board_2432s024")
    endif()

    # T-Deck is an S3 platform
    if(NOT "${IDF_TARGET}" STREQUAL "esp32s3")
        set(EXCLUDE_COMPONENTS "lilygo_tdeck")
    endif()
endif()

project(tactility-root)

add_subdirectory(libs/mlib)
add_subdirectory(tactility-core)

if (NOT IDF_TARGET)
    add_subdirectory(libs/freertos-kernel)
    add_subdirectory(libs/mbedtls)
    add_subdirectory(main_sim)
endif()
