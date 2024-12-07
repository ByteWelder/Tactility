cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (DEFINED ENV{ESP_IDF_VERSION})
    file(GLOB_RECURSE SOURCE_FILES Source/*.c*)

    idf_component_register(
        SRCS ${SOURCE_FILES}
        INCLUDE_DIRS "Source/"
        REQUIRES mbedtls nvs_flash
    )

    add_definitions(-DESP_PLATFORM)
else()
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

    ADD_DEFINITIONS(-d_nULLABLE=)
    ADD_DEFINITIONS(-d_nONNULL=)
    TARGET_LINK_LIBRARIES(TactilityCore
        public MBEDTLS
        public FREERTOS_KERNEL
    )
endif()