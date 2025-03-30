cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (DEFINED ENV{ESP_IDF_VERSION})
    file(GLOB_RECURSE SOURCE_FILES Source/*.c*)

    list(APPEND REQUIRES_LIST TactilityCore lvgl driver elf_loader lv_screenshot QRCode esp_lvgl_port minmea esp_wifi nvs_flash spiffs vfs fatfs lwip)
    if ("${IDF_TARGET}" STREQUAL "esp32s3")
        list(APPEND REQUIRES_LIST esp_tinyusb)
    endif ()

    idf_component_register(
        SRCS ${SOURCE_FILES}
        INCLUDE_DIRS "Include/"
        PRIV_INCLUDE_DIRS "Private/"
        REQUIRES ${REQUIRES_LIST}
    )

    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${COMPONENT_LIB} PUBLIC -Wno-unused-variable)
    endif ()

    if (NOT DEFINED TACTILITY_SKIP_SPIFFS)
        # Read-only
        fatfs_create_rawflash_image(system "${CMAKE_CURRENT_SOURCE_DIR}/../Data/system" FLASH_IN_PROJECT PRESERVE_TIME)
        # Read-write
        fatfs_create_spiflash_image(data "${CMAKE_CURRENT_SOURCE_DIR}/../Data/data" FLASH_IN_PROJECT PRESERVE_TIME)
    endif ()

else()
    file(GLOB_RECURSE SOURCES "Source/*.c*")

    add_library(Tactility OBJECT)

    target_sources(Tactility
        PRIVATE ${SOURCES}
    )

    include_directories(
        PRIVATE Private/
    )

    target_include_directories(Tactility
        PUBLIC Include/
    )

    add_definitions(-D_Nullable=)
    add_definitions(-D_Nonnull=)

    target_link_libraries(Tactility
        PUBLIC TactilityCore
        PUBLIC freertos_kernel
        PUBLIC lvgl
        PUBLIC lv_screenshot
        PUBLIC minmea
    )
endif()

