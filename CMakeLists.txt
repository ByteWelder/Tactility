cmake_minimum_required(VERSION 3.20)

add_definitions(-DTT_DEBUG)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_ASM_COMPILE_OBJECT "${CMAKE_CXX_COMPILER_TARGET}")

include("Buildscripts/logo.cmake")

if (NOT WIN32)
    string(ASCII 27 Esc)
    set(ColorReset "${Esc}[m")
    set(Cyan "${Esc}[36m")
else ()
    set(ColorReset "")
    set(Cyan "")
endif ()

file(READ version.txt TACTILITY_VERSION)
add_compile_definitions(TT_VERSION="${TACTILITY_VERSION}")

if (DEFINED ENV{ESP_IDF_VERSION})
    message("Using ESP-IDF ${Cyan}v$ENV{ESP_IDF_VERSION}${ColorReset}")
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)

    include("Buildscripts/board.cmake")

    init_tactility_globals("sdkconfig")
    get_property(TACTILITY_BOARD_PROJECT GLOBAL PROPERTY TACTILITY_BOARD_PROJECT)

    set(COMPONENTS Firmware)
    set(EXTRA_COMPONENT_DIRS
        "Firmware"
        "Boards/${TACTILITY_BOARD_PROJECT}"
        "Drivers"
        "Tactility"
        "TactilityC"
        "TactilityCore"
        "Libraries/esp_lvgl_port"
        "Libraries/elf_loader"
        "Libraries/epdiy"
        "Libraries/lvgl"
        "Libraries/lv_screenshot"
        "Libraries/minitar"
        "Libraries/minmea"
        "Libraries/QRCode"
    )

    set(EXCLUDE_COMPONENTS "Simulator")

    idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=esp_panic_handler" APPEND)
    idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=lv_button_create" APPEND)
    idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=lv_dropdown_create" APPEND)
    idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=lv_list_create" APPEND)
    idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=lv_list_add_button" APPEND)
    idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=lv_obj_create" APPEND)
    idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=lv_obj_set_flex_flow" APPEND)
    idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=lv_switch_create" APPEND)
    idf_build_set_property(LINK_OPTIONS "-Wl,--wrap=lv_textarea_create" APPEND)

else ()
    message("Building for sim target")
    add_compile_definitions(CONFIG_TT_BOARD_ID="simulator")
    add_compile_definitions(CONFIG_TT_BOARD_NAME="Simulator")
endif ()

project(Tactility)

# Defined as regular project for PC and component for ESP
if (NOT DEFINED ENV{ESP_IDF_VERSION})
    add_subdirectory(Tactility)
    add_subdirectory(TactilityCore)
    add_subdirectory(Boards/simulator)
endif ()

if (NOT DEFINED ENV{ESP_IDF_VERSION})
    # FreeRTOS
    set(FREERTOS_CONFIG_FILE_DIRECTORY ${PROJECT_SOURCE_DIR}/Boards/simulator/Source CACHE STRING "")
    set(FREERTOS_PORT GCC_POSIX CACHE STRING "")

    add_subdirectory(Libraries/cJSON)
    add_subdirectory(Libraries/FreeRTOS-Kernel)
    add_subdirectory(Libraries/lv_screenshot)
    add_subdirectory(Libraries/QRCode)
    add_subdirectory(Libraries/minitar)
    add_subdirectory(Libraries/minmea)
    target_compile_definitions(freertos_kernel PUBLIC "projCOVERAGE_TEST=0")
    target_include_directories(freertos_kernel
        PUBLIC Boards/simulator/Source # for FreeRTOSConfig.h
    )

    # EmbedTLS
    set(ENABLE_TESTING OFF)
    set(ENABLE_PROGRAMS OFF)
    add_subdirectory(Libraries/mbedtls)

    # SDL
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    add_subdirectory(Libraries/SDL) # Added as idf component for ESP and as library for other targets

    # LVGL
    add_compile_definitions($<$<BOOL:${LV_USE_DRAW_SDL}>:LV_USE_DRAW_SDL=1>)
    add_subdirectory(Libraries/lvgl) # Added as idf component for ESP and as library for other targets
    target_link_libraries(lvgl PRIVATE SDL2-static)

    # Sim app
    add_subdirectory(Firmware)

    # Tests
    add_subdirectory(Tests)

endif ()
