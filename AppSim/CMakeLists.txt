cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE SOURCES "Source/*.c*")
add_executable(AppSim ${SOURCES})
target_link_libraries(AppSim
    PRIVATE Tactility
    PRIVATE TactilityCore
    PRIVATE TactilityHeadless
    PRIVATE lvgl
)

find_package(SDL2 REQUIRED CONFIG)
target_link_libraries(AppSim PRIVATE ${SDL2_LIBRARIES})
include_directories(${SDL2_INCLUDE_DIRS})

add_definitions(-D_Nullable=)
add_definitions(-D_Nonnull=)
