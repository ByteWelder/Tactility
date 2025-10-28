file(GLOB_RECURSE SOURCE_FILES Source/*.c*)

idf_component_register(
    SRCS ${SOURCE_FILES}
    INCLUDE_DIRS "Source"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../Libraries/lvgl/src/drivers/display/st7789"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../Libraries/lvgl/src/drivers/display/lcd"
    REQUIRES Tactility lvgl esp_lcd esp_lvgl_port ButtonControl driver PwmBacklight lvgl EstimatedPower espressif__esp_lcd_panel_io_additions
)
