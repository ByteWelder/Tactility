cmake_minimum_required(VERSION 3.16)

add_definitions(-DTT_DEBUG)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_ASM_COMPILE_OBJECT "${CMAKE_CXX_COMPILER_TARGET}")

if (DEFINED ENV{ESP_IDF_VERSION})
    message("Building with ESP-IDF v$ENV{ESP_IDF_VERSION}")
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)

    add_definitions(-DESP_TARGET)
    add_compile_definitions(ESP_TARGET)

    add_definitions(-DARDUINO_M5STACK_CORE2)
    add_compile_definitions(ARDUINO_M5STACK_CORE2)

    set(COMPONENTS App)
    set(EXTRA_COMPONENT_DIRS
        "Boards"
        "App"
        "Tactility"
        "TactilityHeadless"
        "Libraries/esp_lvgl_port"
        "Libraries/lvgl"
        "Libraries/M5Unified"
        "Libraries/M5GFX"
    )

    # ESP32 boards
    if(NOT "${IDF_TARGET}" STREQUAL "esp32")
        set(EXCLUDE_COMPONENTS "YellowBoard")
        set(EXCLUDE_COMPONENTS "M5stackCore2")
        set(EXCLUDE_COMPONENTS "M5stackCoreS3")
    endif()

    if(NOT "${IDF_TARGET}" STREQUAL "Simulator")
        set(EXCLUDE_COMPONENTS "Simulator")
    endif()

    # ESP32-S3 boards
    if(NOT "${IDF_TARGET}" STREQUAL "esp32s3")
        set(EXCLUDE_COMPONENTS "LilygoTdeck")
        set(EXCLUDE_COMPONENTS "WaveshareS3Touch")
    endif()
else()
    message("Building for sim target")
endif()

project(TactilityRoot)

# Defined as regular project for PC and component for ESP
if (NOT DEFINED ENV{ESP_IDF_VERSION})
    add_subdirectory(Tactility)
    add_subdirectory(TactilityHeadless)
    add_subdirectory(Boards/Simulator)
endif()

add_subdirectory(TactilityCore)

add_subdirectory(Libraries/lv_screenshot)

if (NOT DEFINED ENV{ESP_IDF_VERSION})
    # FreeRTOS
    set(FREERTOS_CONFIG_FILE_DIRECTORY ${PROJECT_SOURCE_DIR}/Boards/Simulator/Source CACHE STRING "")
    set(FREERTOS_PORT GCC_POSIX CACHE STRING "")
    add_subdirectory(Libraries/FreeRTOS-Kernel)
    target_compile_definitions(freertos_kernel PUBLIC "projCOVERAGE_TEST=0")
    target_include_directories(freertos_kernel
        PUBLIC Boards/Simulator/Source # for FreeRTOSConfig.h
    )

    # EmbedTLS
    set(ENABLE_TESTING OFF)
    set(ENABLE_PROGRAMS OFF)
    add_subdirectory(Libraries/mbedtls)

    # Sim app
    if (NOT DEFINED ENV{SKIP_SDL})
        add_subdirectory(App)
    endif()

    # Tests
    add_subdirectory(Tests)

    # SDL & LVGL
    set(LV_CONF_PATH ${PROJECT_SOURCE_DIR}/Boards/Simulator/Source/lv_conf.h)
    add_subdirectory(Libraries/lvgl) # Added as idf component for ESP and as library for other targets
    target_include_directories(lvgl
        PUBLIC Boards/Source # for lv_conf.h and lv_drv_conf.h
    )

    # TODO: This is a temporary skipping option for running unit tests
    # TODO: Remove when github action for SDL is working again
    if (NOT DEFINED ENV{SKIP_SDL})
        find_package(SDL2 REQUIRED CONFIG)

        option(LV_USE_DRAW_SDL "Use SDL draw unit" OFF)
        option(LV_USE_LIBPNG "Use libpng to decode PNG" OFF)
        option(LV_USE_LIBJPEG_TURBO "Use libjpeg turbo to decode JPEG" OFF)
        option(LV_USE_FFMPEG "Use libffmpeg to display video using lv_ffmpeg" OFF)
        option(LV_USE_FREETYPE "Use freetype lib" OFF)

        add_compile_definitions($<$<BOOL:${LV_USE_DRAW_SDL}>:LV_USE_DRAW_SDL=1>)
        add_compile_definitions($<$<BOOL:${LV_USE_LIBPNG}>:LV_USE_LIBPNG=1>)
        add_compile_definitions($<$<BOOL:${LV_USE_LIBJPEG_TURBO}>:LV_USE_LIBJPEG_TURBO=1>)
        add_compile_definitions($<$<BOOL:${LV_USE_FFMPEG}>:LV_USE_FFMPEG=1>)

        if (LV_USE_DRAW_SDL)
            set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
            # Need to install libsdl2-image-dev
            # `sudo apt install libsdl2-image-dev`
            # `brew install sdl2_image`
            find_package(SDL2_image REQUIRED)
            target_include_directories(lvgl PUBLIC ${SDL2_IMAGE_INCLUDE_DIRS})
            target_link_libraries(AppSim ${SDL2_IMAGE_LIBRARIES})
            target_link_libraries(Simulator ${SDL2_IMAGE_LIBRARIES})
        endif(LV_USE_DRAW_SDL)

        if (LV_USE_LIBPNG)
            find_package(PNG REQUIRED)
            target_include_directories(lvgl PUBLIC ${PNG_INCLUDE_DIR})
            target_link_libraries(AppSim ${PNG_LIBRARY})
        endif(LV_USE_LIBPNG)

        if (LV_USE_LIBJPEG_TURBO)
            # Need to install libjpeg-turbo8-dev
            # `sudo apt install libjpeg-turbo8-dev`
            # `brew install libjpeg-turbo`
            find_package(JPEG REQUIRED)
            target_include_directories(lvgl PUBLIC ${JPEG_INCLUDE_DIRS})
            target_link_libraries(AppSim ${JPEG_LIBRARIES})
        endif(LV_USE_LIBJPEG_TURBO)

        if (LV_USE_FFMPEG)
            target_link_libraries(main avformat avcodec avutil swscale)
        endif(LV_USE_FFMPEG)

        if (LV_USE_FREETYPE)
            find_package(Freetype REQUIRED)
            target_link_libraries(AppSim ${FREETYPE_LIBRARIES})
            target_include_directories(lvgl PUBLIC ${FREETYPE_INCLUDE_DIRS})
        endif(LV_USE_FREETYPE)
    endif()
endif()
