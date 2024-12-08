cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (DEFINED ENV{ESP_IDF_VERSION})
    file(GLOB_RECURSE SOURCE_FILES Source/*.c*)

    idf_component_register(
        SRCS ${SOURCE_FILES}
        INCLUDE_DIRS "src/"
        REQUIRES lvgl
    )

    add_definitions(-DESP_PLATFORM)
else()
    file(GLOB SOURCES "src/*.c*")
    file(GLOB HEADERS "src/*.h*")

    add_library(lv_screenshot STATIC)

    target_sources(lv_screenshot
        PRIVATE ${SOURCES}
        PUBLIC ${HEADERS}
    )

    target_include_directories(lv_screenshot
        PRIVATE private
        PUBLIC src
    )

    target_link_libraries(lv_screenshot
        PUBLIC lvgl
    )
endif()