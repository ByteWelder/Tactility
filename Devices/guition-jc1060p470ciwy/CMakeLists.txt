file(GLOB_RECURSE SOURCE_FILES Source/*.c*)

idf_component_register(
    SRCS ${SOURCE_FILES}
    INCLUDE_DIRS "Source"
    REQUIRES Tactility esp_lvgl_port esp_lcd EspLcdCompat esp_lcd_jd9165 GT911 PwmBacklight driver vfs fatfs
    PRIV_REQUIRES esp_adc EstimatedPower
)
