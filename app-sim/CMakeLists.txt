cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE SOURCES "src/*.c")
add_executable(app-sim ${SOURCES})
target_link_libraries(app-sim
    PRIVATE tactility
    PRIVATE tactility-core
    PRIVATE lvgl
    PRIVATE lv_drivers
    PRIVATE SDL2
)

target_link_libraries(app-sim ${SDL2_LIBRARY})
include_directories(${SDL2_INCLUDE_DIR})

add_definitions(-D_Nullable=)
add_definitions(-D_Nonnull=)
