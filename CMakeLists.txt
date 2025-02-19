cmake_minimum_required(VERSION 3.20)

add_definitions(-DTT_DEBUG)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_ASM_COMPILE_OBJECT "${CMAKE_CXX_COMPILER_TARGET}")

include("Buildscripts/logo.cmake")

file(READ version.txt TACTILITY_VERSION)
add_compile_definitions(TT_VERSION="${TACTILITY_VERSION}")

if (DEFINED ENV{ESP_IDF_VERSION})
    message("Building with ESP-IDF v$ENV{ESP_IDF_VERSION}")
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)

    set(COMPONENTS App)
    set(EXTRA_COMPONENT_DIRS
        "App"
        "Boards"
        "Drivers"
        "Tactility"
        "TactilityC"
        "TactilityCore"
        "TactilityHeadless"
        "Libraries/esp_lvgl_port"
        "Libraries/elf_loader"
        "Libraries/lvgl"
        "Libraries/lv_screenshot"
        "Libraries/minmea"
        "Libraries/QRCode"
    )

    set(EXCLUDE_COMPONENTS "Simulator")

    # Non-ESP32 boards
    if(NOT "${IDF_TARGET}" STREQUAL "esp32")
        set(EXCLUDE_COMPONENTS "CYD-2432S024C")
        set(EXCLUDE_COMPONENTS "ElecrowCrowpanelBasic28")
        set(EXCLUDE_COMPONENTS "M5stackCore2")
    endif()

    # Non-ESP32-S3 boards
    if(NOT "${IDF_TARGET}" STREQUAL "esp32s3")
        set(EXCLUDE_COMPONENTS "ElecrowCrowpanelAdvance28")
        set(EXCLUDE_COMPONENTS "LilygoTdeck")
        set(EXCLUDE_COMPONENTS "M5stackCoreS3")
        set(EXCLUDE_COMPONENTS "UnPhone")
    endif()

    # LVGL
    get_filename_component(
        LVGL_CONFIG_FULL_PATH Libraries/lvgl_conf ABSOLUTE
    )

    add_compile_definitions(LV_CONF_PATH="${LVGL_CONFIG_FULL_PATH}/lv_conf_kconfig.h")
    idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=esp_panic_handler" APPEND)
    idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=esp_log_write" APPEND)
else()
    message("Building for sim target")
    add_compile_definitions(CONFIG_TT_BOARD_ID="simulator")
    add_compile_definitions(CONFIG_TT_BOARD_NAME="Simulator")
endif()

project(Tactility)

# Defined as regular project for PC and component for ESP
if (NOT DEFINED ENV{ESP_IDF_VERSION})
    add_subdirectory(Tactility)
    add_subdirectory(TactilityCore)
    add_subdirectory(TactilityHeadless)
    add_subdirectory(Boards/Simulator)
endif()

if (NOT DEFINED ENV{ESP_IDF_VERSION})
    # FreeRTOS
    set(FREERTOS_CONFIG_FILE_DIRECTORY ${PROJECT_SOURCE_DIR}/Boards/Simulator/Source CACHE STRING "")
    set(FREERTOS_PORT GCC_POSIX CACHE STRING "")
    add_subdirectory(Libraries/FreeRTOS-Kernel)
    add_subdirectory(Libraries/lv_screenshot)
    add_subdirectory(Libraries/QRCode)
    add_subdirectory(Libraries/minmea)
    target_compile_definitions(freertos_kernel PUBLIC "projCOVERAGE_TEST=0")
    target_include_directories(freertos_kernel
        PUBLIC Boards/Simulator/Source # for FreeRTOSConfig.h
    )

    # EmbedTLS
    set(ENABLE_TESTING OFF)
    set(ENABLE_PROGRAMS OFF)
    add_subdirectory(Libraries/mbedtls)

    # Sim app
    add_subdirectory(App)

    # Tests
    add_subdirectory(Tests)

    # SDL
    add_compile_definitions($<$<BOOL:${LV_USE_DRAW_SDL}>:LV_USE_DRAW_SDL=1>)
    add_subdirectory(Libraries/SDL) # Added as idf component for ESP and as library for other targets

    # LVGL
    add_subdirectory(Libraries/lvgl) # Added as idf component for ESP and as library for other targets
    include_directories(lvgl PUBLIC ${PROJECT_SOURCE_DIR}/Libraries/lvgl_conf)
    target_compile_definitions(lvgl PUBLIC "-DLV_CONF_PATH=\"${PROJECT_SOURCE_DIR}/Libraries/lvgl_conf/lv_conf_simulator.h\"")
    target_link_libraries(lvgl PRIVATE SDL2-static)

endif()
