idf_component_register(
    SRC_DIRS "source"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "private"
    REQUIRES tactility esp_lvgl_port esp_lcd_ili9341 driver vfs fatfs M5Unified
)
