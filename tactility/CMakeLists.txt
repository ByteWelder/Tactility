cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

if ($ENV{ESP_IDF_VERSION})
    add_definitions(-DESP_PLATFORM)
    target_link_libraries(tactility
        PUBLIC tactility-core
        PUBLIC idf::lvgl # libs/
        PUBLIC idf::driver
        PUBLIC idf::spiffs
    )
else()
    add_definitions(-D_Nullable=)
    add_definitions(-D_Nonnull=)
    target_link_libraries(tactility
        PUBLIC tactility-core
        PUBLIC lvgl
        PUBLIC freertos-kernel
    )
endif()

