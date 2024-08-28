cmake_minimum_required(VERSION 3.16)

set(BOARD_COMPONENTS tactility-esp)

if("${IDF_TARGET}" STREQUAL "esp32")
    list(APPEND BOARD_COMPONENTS
        yellow_board
        m5stack_core2
    )
endif()

# T-Deck is an S3 platform
if("${IDF_TARGET}" STREQUAL "esp32s3")
    list(APPEND BOARD_COMPONENTS
        lilygo_tdeck
        waveshare_s3_touch
    )
endif()

idf_component_register(
    SRC_DIRS "src"
        "src/hello_world"
    REQUIRES ${BOARD_COMPONENTS}
)

target_link_libraries(${COMPONENT_LIB} ${IDF_TARGET_NAME} tactility)
