idf_component_register(
    SRC_DIRS "Source" "Source/hal"
    INCLUDE_DIRS "Source"
    REQUIRES Tactility esp_lvgl_port esp_lcd esp_lcd_touch_gt911 driver vfs fatfs
)
