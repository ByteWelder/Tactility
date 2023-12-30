idf_component_register(
    SRC_DIRS "src"
    "src/apps/system/system_info"
    "src/apps/services/desktop"
    "src/apps/services/loader"
    "src/apps/services/gui"
    INCLUDE_DIRS "src"
    REQUIRES esp_lvgl_port esp_lcd esp_lcd_touch driver mlib cmsis_core furi
)
