cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB SOURCES "src/*.c")
file(GLOB HEADERS "src/*.h")

add_library(tactility-headless OBJECT)

target_sources(tactility-headless
    PRIVATE ${SOURCES}
    PUBLIC ${HEADERS}
)

target_include_directories(tactility-headless
    PRIVATE src/
    INTERFACE src/
)

target_link_libraries(tactility-headless
    PUBLIC tactility-core
)

if (DEFINED ENV{ESP_IDF_VERSION})
    add_definitions(-DESP_PLATFORM)
else()
    add_definitions(-D_Nullable=)
    add_definitions(-D_Nonnull=)
endif()
