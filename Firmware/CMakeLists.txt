cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE SOURCE_FILES "Source/*.c*")

if (DEFINED ENV{ESP_IDF_VERSION})
    # Read device id/project
    include("../Buildscripts/device.cmake")
    init_tactility_globals("../sdkconfig")
    get_property(TACTILITY_DEVICE_PROJECT GLOBAL PROPERTY TACTILITY_DEVICE_PROJECT)

    idf_component_register(
        SRCS ${SOURCE_FILES}
        REQUIRES ${DEVICE_COMPONENTS}
        REQUIRES Tactility TactilityC ${TACTILITY_DEVICE_PROJECT}
    )
else ()

    add_executable(FirmwareSim ${SOURCE_FILES})
    target_link_libraries(FirmwareSim
        PRIVATE Tactility
        PRIVATE TactilityCore
        PRIVATE Simulator
        PRIVATE SDL2::SDL2-static SDL2-static
    )

    add_definitions(-D_Nullable=)
    add_definitions(-D_Nonnull=)
endif ()
