cmake_minimum_required(VERSION 3.20)

if (EMSCRIPTEN)

    file(GLOB_RECURSE SOURCES "Source/*.c*")
    file(GLOB_RECURSE HEADERS "Source/*.h*")

    add_library(websimulator OBJECT)

    target_sources(websimulator
        PRIVATE ${SOURCES}
        PUBLIC ${HEADERS}
    )

    target_link_libraries(websimulator
        PRIVATE TactilityKernel
        PRIVATE TactilityFreeRtos
        PRIVATE lvgl
        PRIVATE SDL2-static
    )

    target_link_libraries(websimulator PRIVATE ${SDL2_LIBRARIES})

endif()
