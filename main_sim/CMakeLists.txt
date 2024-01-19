cmake_minimum_required(VERSION 3.16)

file(GLOB SOURCES "src/*.c")
add_executable(main_sim ${SOURCES})
target_link_libraries(main_sim PRIVATE tactility-core)
target_link_libraries(main_sim PRIVATE freertos-kernel)

