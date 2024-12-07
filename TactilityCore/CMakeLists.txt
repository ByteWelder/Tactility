cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES "Source/*.c*")
file(GLOB_RECURSE HEADERS "Source/*.h*")

add_library(TactilityCore OBJECT)

target_sources(TactilityCore
    PRIVATE ${SOURCES}
    PUBLIC ${HEADERS}
)

target_include_directories(TactilityCore SYSTEM
    PUBLIC Source/
)

if (DEFINED ENV{ESP_IDF_VERSION})
    add_definitions(-DESP_PLATFORM)
    target_link_libraries(TactilityCore
        PUBLIC idf::mbedtls
        PRIVATE idf::nvs_flash # ESP-IDF // for secure.c
    )
else()
    add_definitions(-D_Nullable=)
    add_definitions(-D_Nonnull=)
    target_link_libraries(TactilityCore
    	PUBLIC mbedtls
    	PUBLIC freertos_kernel
    )
endif()
