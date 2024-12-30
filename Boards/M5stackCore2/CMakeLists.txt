idf_component_register(
    SRC_DIRS "Source" "Source/hal" "Source/ft6x36"
    INCLUDE_DIRS "Source"
    REQUIRES Tactility esp_lvgl_port esp_lcd esp_lcd_ili9341 driver vfs fatfs M5Unified
)
