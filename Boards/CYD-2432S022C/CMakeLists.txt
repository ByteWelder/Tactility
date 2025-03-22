file(GLOB_RECURSE SOURCE_FILES Source/*.c*)

idf_component_register(
    SRCS ${SOURCE_FILES}
    INCLUDE_DIRS "Source" "/opt/esp/idf/components/esp_lcd/rgb/include"
    REQUIRES Tactility lvgl driver vfs fatfs RgbDisplay PwmBacklight esp_lcd
)
