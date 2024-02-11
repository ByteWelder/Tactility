cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB SOURCES "src/*.c")
file(GLOB HEADERS "src/*.h")

add_library(lv_screenshot STATIC)

target_sources(lv_screenshot
    PRIVATE ${SOURCES}
    PUBLIC ${HEADERS}
)

target_include_directories(lv_screenshot
    PRIVATE private
    PUBLIC src
)

if (DEFINED ENV{ESP_IDF_VERSION})
    target_link_libraries(lv_screenshot
        PUBLIC idf::lvgl
    )
else()
    target_link_libraries(lv_screenshot
        PUBLIC lvgl
    )
endif()