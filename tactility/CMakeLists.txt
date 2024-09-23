cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (DEFINED ENV{ESP_IDF_VERSION})
    file(GLOB_RECURSE SOURCE_FILES src/*.c)

    idf_component_register(
        SRCS ${SOURCE_FILES}
        INCLUDE_DIRS "src/"
        REQUIRES tactility-headless lvgl esp_driver_gpio
    )

    target_link_libraries(${COMPONENT_LIB}
        PUBLIC lv_screenshot
    )

    add_definitions(-DESP_PLATFORM)
else()
    file(GLOB_RECURSE SOURCES "src/*.c")
    file(GLOB_RECURSE HEADERS "src/*.h")

    add_library(tactility OBJECT)
    target_sources(tactility
        PRIVATE ${SOURCES}
        PUBLIC ${HEADERS}
    )

    target_include_directories(tactility
        PRIVATE src/
        INTERFACE src/
    )

    add_definitions(-D_Nullable=)
    add_definitions(-D_Nonnull=)
    target_link_libraries(tactility
        PUBLIC tactility-headless
        PUBLIC lvgl
        PUBLIC freertos_kernel
        PUBLIC lv_screenshot
    )
endif()

