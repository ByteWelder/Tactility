cmake_minimum_required(VERSION 3.16)

if (DEFINED ENV{ESP_IDF_VERSION})
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
else()

    file(GLOB_RECURSE SOURCES "Source/*.c*")
    add_executable(AppSim ${SOURCES})
    target_link_libraries(AppSim
        PRIVATE Tactility
        PRIVATE TactilityCore
        PRIVATE TactilityHeadless
        PRIVATE Simulator
    )

    find_package(SDL2 REQUIRED CONFIG)
    target_link_libraries(AppSim PRIVATE ${SDL2_LIBRARIES})
    include_directories(${SDL2_INCLUDE_DIRS})

    add_definitions(-D_Nullable=)
    add_definitions(-D_Nonnull=)
endif()
