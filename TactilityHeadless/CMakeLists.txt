cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (DEFINED ENV{ESP_IDF_VERSION})
    file(GLOB_RECURSE SOURCE_FILES Source/*.c*)

    idf_component_register(
        SRCS ${SOURCE_FILES}
        INCLUDE_DIRS "Source/"
        PRIV_INCLUDE_DIRS "Private/"
        REQUIRES TactilityCore esp_wifi nvs_flash spiffs driver
    )

    if (NOT DEFINED TACTILITY_SKIP_SPIFFS)
        set(ASSETS_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Data/assets")
        spiffs_create_partition_image(assets ${ASSETS_SRC_DIR} FLASH_IN_PROJECT)
        set(CONFIG_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Data/config")
        spiffs_create_partition_image(config ${CONFIG_SRC_DIR} FLASH_IN_PROJECT)
    endif()

    add_definitions(-DESP_PLATFORM)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${COMPONENT_LIB} PUBLIC -Wno-unused-variable)
    endif()
else()
    file(GLOB_RECURSE SOURCES "Source/*.c*")
    file(GLOB_RECURSE HEADERS "Source/*.h*")

    add_library(TactilityHeadless OBJECT)
    target_sources(TactilityHeadless
        PRIVATE ${SOURCES}
        PUBLIC ${HEADERS}
    )

    include_directories(
        Private/
    )

    target_include_directories(TactilityHeadless
        PUBLIC Source/
    )

    add_definitions(-D_Nullable=)
    add_definitions(-D_Nonnull=)
    target_link_libraries(TactilityHeadless
        PUBLIC TactilityCore
        PUBLIC freertos_kernel
    )
endif()
