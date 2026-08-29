cmake_minimum_required(VERSION 3.20)

include("${CMAKE_CURRENT_LIST_DIR}/../../Buildscripts/module.cmake")

file(GLOB_RECURSE SOURCE_FILES "source/*.c*")

list(APPEND REQUIRES_LIST
    TactilityKernel
)

if (NOT DEFINED ENV{ESP_IDF_VERSION})
    find_package(Threads REQUIRED)
    list(APPEND REQUIRES_LIST
        Threads::Threads
    )
endif ()

tactility_add_module(pthread-module
    SRCS ${SOURCE_FILES}
    INCLUDE_DIRS include/
    REQUIRES ${REQUIRES_LIST}
)
