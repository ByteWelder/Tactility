cmake_minimum_required(VERSION 3.16)

add_definitions(-DFURI_DEBUG)

set(COMPONENTS main)
set(EXTRA_COMPONENT_DIRS "boards" "components")

# Yellow Board only runs on ESP32
if(NOT "${IDF_TARGET}" STREQUAL "esp32")
    set(EXCLUDE_COMPONENTS "yellow_board_2432s024")
endif()

# T-Deck is an S3 platform
if(NOT "${IDF_TARGET}" STREQUAL "esp32s3")
    set(EXCLUDE_COMPONENTS "lilygo_tdeck")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Tactility)
