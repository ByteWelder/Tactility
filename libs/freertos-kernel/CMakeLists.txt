# Based on: https://github.com/jonathanmichel/FreeRTOS-CMake/blob/master/CMakeLists.txt

cmake_minimum_required(VERSION 3.5)
project(freertos-kernel C)

set(FREERTOS_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(KERNEL_DIR ${FREERTOS_DIR}/portable/ThirdParty/GCC/Posix)

include_directories(
    .
    ${FREERTOS_DIR}/include
    ${KERNEL_DIR}
    ${KERNEL_DIR}/utils
)

# FreeRTOS sources
FILE(GLOB FREERTOS_SOURCES ${FREERTOS_DIR}/*.c)

# Memory manager (use malloc() / free())
list(APPEND FREERTOS_SOURCES "${FREERTOS_DIR}/portable/MemMang/heap_3.c")

# Port sources
FILE(GLOB KERNEL_SOURCES ${KERNEL_DIR}/*.c)
list(APPEND FREERTOS_SOURCES ${KERNEL_SOURCES})
FILE(GLOB KERNEL_SOURCES ${KERNEL_DIR}/utils/*.c)
list(APPEND FREERTOS_SOURCES ${KERNEL_SOURCES})

# Link FreeRTOS as a static library
add_library(freertos-kernel STATIC ${FREERTOS_SOURCES})

target_sources(freertos-kernel 
    PRIVATE ${FREERTOS_SOURCES}
    PUBLIC ${FREERTOS_HEADERS}
)
target_include_directories(freertos-kernel
    PRIVATE ${FREERTOS_DIR}/include
    INTERFACE ${FREERTOS_DIR}/include
    INTERFACE ${KERNEL_DIR}
)
