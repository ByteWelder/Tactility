# Include SIMD assembly source code for rendering
if(CONFIG_IDF_TARGET_ESP32 OR CONFIG_IDF_TARGET_ESP32S3)
    message(VERBOSE "Compiling SIMD")
    set(PORT_PATH "../../../src/lvgl9")

    if(CONFIG_IDF_TARGET_ESP32S3)
        file(GLOB_RECURSE ASM_SOURCES ${PORT_PATH}/simd/*_esp32s3.S)    # Select only esp32s3 related files
    else()
        file(GLOB_RECURSE ASM_SOURCES ${PORT_PATH}/simd/*_esp32.S)      # Select only esp32 related files
    endif()
else()
    message(WARNING "This test app is intended only for esp32 and esp32s3")
endif()

# Hard copy of LV files
file(GLOB_RECURSE BLEND_SRCS lv_blend/src/*.c)

idf_component_register(SRCS "test_app_main.c" "test_lv_fill_functionality.c" "test_lv_fill_benchmark.c" ${BLEND_SRCS} ${ASM_SOURCES}
                      INCLUDE_DIRS "lv_blend/include" "../../../include"
                      REQUIRES unity
                      WHOLE_ARCHIVE)
