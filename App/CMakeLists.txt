cmake_minimum_required(VERSION 3.20)

if (DEFINED ENV{ESP_IDF_VERSION})
    set(BOARD_COMPONENTS Tactility TactilityC)

    if("${IDF_TARGET}" STREQUAL "esp32")
        list(APPEND BOARD_COMPONENTS
            CYD-2432S024C
            ElecrowCrowpanelBasic28
            ElecrowCrowpanelBasic35
            M5stackCore2
            CYD-JC2432W328C
        )
    endif()

    if("${IDF_TARGET}" STREQUAL "esp32s3")
        list(APPEND BOARD_COMPONENTS
            ElecrowCrowpanelAdvance28
            ElecrowCrowpanelAdvance35
            LilygoTdeck
            M5stackCoreS3
            UnPhone
            CYD-8048S043C
            CYD-JC8048W550C
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
        PRIVATE SDL2::SDL2-static SDL2-static
    )

    add_definitions(-D_Nullable=)
    add_definitions(-D_Nonnull=)
endif()
