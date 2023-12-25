idf_component_register(
    SRC_DIRS "src"
        "src/applications/main/system_info"
    INCLUDE_DIRS "inc"
    PRIV_INCLUDE_DIRS "src"
    REQUIRES esp_lvgl_port esp_lcd esp_lcd_touch driver
)
