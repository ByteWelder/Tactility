cmake_minimum_required(VERSION 3.20)

include("${CMAKE_CURRENT_LIST_DIR}/../../Buildscripts/module.cmake")

file(GLOB_RECURSE SOURCE_FILES "source/*.c*")

list(APPEND REQUIRES_LIST
    TactilityKernel
)

if (DEFINED ENV{ESP_IDF_VERSION})
    list(APPEND REQUIRES_LIST
        esp_http_client
        lwip
        mbedtls
    )
    list(FILTER SOURCE_FILES EXCLUDE REGEX ".*download_mock\\.cpp$")
else ()
    list(FILTER SOURCE_FILES EXCLUDE REGEX ".*download_esp\\.cpp$")
endif ()

tactility_add_module(http-module
    SRCS ${SOURCE_FILES}
    PRIV_INCLUDE_DIRS private/
    INCLUDE_DIRS include/
    REQUIRES ${REQUIRES_LIST}
)
