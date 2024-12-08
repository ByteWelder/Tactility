cmake_minimum_required(VERSION 3.20)

if (NOT DEFINED ENV{ESP_IDF_VERSION})

    file(GLOB_RECURSE SOURCES "Source/*.c*")
    file(GLOB_RECURSE HEADERS "Source/*.h*")

    add_library(Simulator OBJECT)

    target_sources(Simulator
        PRIVATE ${SOURCES}
        PUBLIC ${HEADERS}
    )
    target_link_libraries(Simulator
        PRIVATE Tactility
        PRIVATE TactilityCore
        PRIVATE TactilityHeadless
        PRIVATE lvgl
    )

    add_definitions(-D_Nullable=)
    add_definitions(-D_Nonnull=)

endif()
