idf_component_register(
    SRC_DIRS "Source" "Source/hal"
    INCLUDE_DIRS "Source"
    REQUIRES Tactility esp_lvgl_port esp_lcd_touch_cst816s ILI934x driver vfs fatfs
)
