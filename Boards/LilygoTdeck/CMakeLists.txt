file(GLOB_RECURSE SOURCE_FILES Source/*.c*)

idf_component_register(
    SRCS ${SOURCE_FILES}
    INCLUDE_DIRS "Source"
    REQUIRES Tactility esp_lvgl_port esp_lcd esp_lcd_touch_gt911 ST7789 PwmBacklight driver esp_adc
)
