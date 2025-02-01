cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (DEFINED ENV{ESP_IDF_VERSION})
    file(GLOB_RECURSE SOURCE_FILES Source/*.c*)

    list(APPEND REQUIRES_LIST TactilityCore esp_wifi nvs_flash driver spiffs vfs fatfs lwip)
    if("${IDF_TARGET}" STREQUAL "esp32s3")
        list(APPEND REQUIRES_LIST esp_tinyusb)
    endif()

    idf_component_register(
        SRCS ${SOURCE_FILES}
        INCLUDE_DIRS "Source/"
        PRIV_INCLUDE_DIRS "Private/"
        REQUIRES ${REQUIRES_LIST}
    )

    if (NOT DEFINED TACTILITY_SKIP_SPIFFS)
        # Read-only
        fatfs_create_rawflash_image(system "${CMAKE_CURRENT_SOURCE_DIR}/../Data/system" FLASH_IN_PROJECT PRESERVE_TIME)
        # Read-write
        fatfs_create_spiflash_image(data "${CMAKE_CURRENT_SOURCE_DIR}/../Data/data" FLASH_IN_PROJECT PRESERVE_TIME)
    endif()

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
