cmake_minimum_required(VERSION 3.16)

file(GLOB SOURCES "src/*.c")
add_executable(app-sim ${SOURCES})
target_link_libraries(app-sim PRIVATE tactility-core)
target_link_libraries(app-sim PRIVATE freertos-kernel)

