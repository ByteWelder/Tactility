cmake_minimum_required(VERSION 3.16)

set(BOARD_COMPONENTS Tactility)

if("${IDF_TARGET}" STREQUAL "esp32")
    list(APPEND BOARD_COMPONENTS
        YellowBoard
        M5stackCore2
    )
endif()

# T-Deck is an S3 platform
if("${IDF_TARGET}" STREQUAL "esp32s3")
    list(APPEND BOARD_COMPONENTS
        LilygoTdeck
        M5stackCoreS3
        WaveshareS3Touch
    )
endif()

idf_component_register(
    SRC_DIRS "Source"
        "Source/HelloWorld"
    REQUIRES ${BOARD_COMPONENTS}
)
