cmake_minimum_required(VERSION 3.16)

add_definitions(-DTT_DEBUG)

if (ESP_PLATFORM)
    message("Building with ESP-IDF v$ENV{ESP_IDF_VERSION}")
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)

    add_definitions(-DESP_TARGET)
    set(COMPONENTS app-esp)
    set(EXTRA_COMPONENT_DIRS
        "boards"
        "tactility-esp"
        "app-esp"
        "libs/esp_lvgl_port"
        "libs/lvgl"
    )

    # Yellow Board only runs on ESP32
    if(NOT "${IDF_TARGET}" STREQUAL "esp32")
        set(EXCLUDE_COMPONENTS "yellow_board_2432s024")
    endif()

    # T-Deck is an S3 platform
    if(NOT "${IDF_TARGET}" STREQUAL "esp32s3")
        set(EXCLUDE_COMPONENTS "lilygo_tdeck")
    endif()
else()
    message("Building for sim target")
endif()

project(tactility-root)

add_subdirectory(libs/mlib)
add_subdirectory(tactility)
add_subdirectory(tactility-core)

if (NOT ESP_PLATFORM)
    add_subdirectory(libs/freertos-kernel)
    target_include_directories(freertos-kernel
        PUBLIC app-sim/src # for FreeRTOSConfig.h
    )

    add_subdirectory(libs/mbedtls)
    add_subdirectory(app-sim)

    # region SDL & LVGL

    find_package(SDL2 REQUIRED CONFIG)

    add_subdirectory(libs/lvgl) # Added as idf component for ESP and as library for other targets
    target_include_directories(lvgl
        PUBLIC ${SDL2_INCLUDE_DIRS}
        PUBLIC app-sim/src # for lv_conf.h and lv_drv_conf.h
    )

    add_subdirectory(libs/lv_drivers)

    option(LV_USE_DRAW_SDL "Use SDL draw unit" OFF)
    option(LV_USE_LIBPNG "Use libpng to decode PNG" OFF)
    option(LV_USE_LIBJPEG_TURBO "Use libjpeg turbo to decode JPEG" OFF)
    option(LV_USE_FFMPEG "Use libffmpeg to display video using lv_ffmpeg" OFF)
    option(LV_USE_FREETYPE "Use freetype lib" OFF)

    set(CMAKE_C_STANDARD 99) # C99 # lvgl officially support C99 and above
    set(CMAKE_CXX_STANDARD 17) # C17
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

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
        target_link_libraries(app-sim ${SDL2_IMAGE_LIBRARIES})
    endif(LV_USE_DRAW_SDL)

    if (LV_USE_LIBPNG)
        find_package(PNG REQUIRED)
        target_include_directories(lvgl PUBLIC ${PNG_INCLUDE_DIR})
        target_link_libraries(app-sim ${PNG_LIBRARY})
    endif(LV_USE_LIBPNG)

    if (LV_USE_LIBJPEG_TURBO)
        # Need to install libjpeg-turbo8-dev
        # `sudo apt install libjpeg-turbo8-dev`
        # `brew install libjpeg-turbo`
        find_package(JPEG REQUIRED)
        target_include_directories(lvgl PUBLIC ${JPEG_INCLUDE_DIRS})
        target_link_libraries(app-sim ${JPEG_LIBRARIES})
    endif(LV_USE_LIBJPEG_TURBO)

    if (LV_USE_FFMPEG)
        target_link_libraries(main avformat avcodec avutil swscale)
    endif(LV_USE_FFMPEG)

    if (LV_USE_FREETYPE)
        find_package(Freetype REQUIRED)
        target_link_libraries(app-sim ${FREETYPE_LIBRARIES})
        target_include_directories(lvgl PUBLIC ${FREETYPE_INCLUDE_DIRS})
    endif(LV_USE_FREETYPE)

    # endregion SDL & LVGL
endif()
