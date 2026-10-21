cmake_minimum_required(VERSION 3.20)

include("${CMAKE_CURRENT_LIST_DIR}/../../Buildscripts/module.cmake")

file(GLOB_RECURSE SOURCE_FILES "source/*.c*")

tactility_add_module(app-hub-module
    SRCS ${SOURCE_FILES}
    PRIV_INCLUDE_DIRS private/
    INCLUDE_DIRS include/
    REQUIRES TactilityKernel app-module service-module http-module cjson-module
)

if (NOT DEFINED ENV{ESP_IDF_VERSION})
    # Matches the app binary folder name of app-posix-module, e.g. "posix-x86_64"
    target_compile_definitions(app-hub-module PRIVATE "TACTILITY_POSIX_ARCH=\"${CMAKE_SYSTEM_PROCESSOR}\"")
endif ()
