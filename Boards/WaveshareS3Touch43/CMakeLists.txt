file(GLOB_RECURSE SOURCE_FILES Source/*.c*)

idf_component_register(
    SRCS ${SOURCE_FILES}
    INCLUDE_DIRS "Source" "/opt/esp/idf/components/esp_lcd/rgb/include"
    REQUIRES Tactility esp_lvgl_port RgbDisplay GT911 driver
)
