cmake_minimum_required(VERSION 3.16)

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

    find_package(SDL2 REQUIRED CONFIG)
    target_link_libraries(Simulator PRIVATE ${SDL2_LIBRARIES})
    include_directories(${SDL2_INCLUDE_DIRS})

    add_definitions(-D_Nullable=)
    add_definitions(-D_Nonnull=)

endif()
