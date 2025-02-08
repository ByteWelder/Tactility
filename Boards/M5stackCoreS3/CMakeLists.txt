idf_component_register(
    SRC_DIRS "Source" "Source/hal" "Source/Axp2101" "Source/Aw9523"
    INCLUDE_DIRS "Source"
    REQUIRES Tactility esp_lvgl_port esp_lcd esp_lcd_ili9341 ILI934x esp_lcd_touch_ft5x06 driver vfs fatfs
)
