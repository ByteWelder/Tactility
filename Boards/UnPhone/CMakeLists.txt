idf_component_register(
    SRC_DIRS "Source" "Source/hal" "Source/hx8357" "Source/bq24295"
    INCLUDE_DIRS "Source"
    REQUIRES Tactility esp_lvgl_port esp_io_expander esp_io_expander_tca95xx_16bit esp_lcd_touch esp_lcd_touch_xpt2046
)
