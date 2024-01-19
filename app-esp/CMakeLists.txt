cmake_minimum_required(VERSION 3.16)

set(BOARD_COMPONENTS tactility)

if("${IDF_TARGET}" STREQUAL "esp32")
    list(APPEND BOARD_COMPONENTS yellow_board)
endif()

# T-Deck is an S3 platform
if("${IDF_TARGET}" STREQUAL "esp32s3")
    list(APPEND BOARD_COMPONENTS lilygo_tdeck)
endif()

idf_component_register(
    SRC_DIRS "src"
        "src/hello_world"
    REQUIRES ${BOARD_COMPONENTS}
)

