cmake_minimum_required(VERSION 3.16)

add_definitions(-DTT_DEBUG)

#if (IDF_TARGET)
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

add_subdirectory(libs/mlib)
add_subdirectory(libs/tactility-core)

#else()
#project(Tactility)
#add_executable(Tactility main/src/main.c main/src/hello_world/hello_world.c)
#add_subdirectory(libs/mlib)
#add_subdirectory(libs/mbedtls)
#endif()

