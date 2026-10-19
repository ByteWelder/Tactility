cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE SOURCES "source/*.c**" "module.cpp" "freertos/freertos.c")

add_library(platform-web OBJECT)
target_sources(platform-web PRIVATE ${SOURCES})
target_include_directories(platform-web PUBLIC freertos/)
target_link_libraries(platform-web PUBLIC TactilityKernel)
