idf_component_register(
    SRC_DIRS "src"
    "src/applications"
    "src/applications/system/system_info"
    "src/applications/services/desktop"
    "src/applications/services/loader"
    "src/applications/services/gui"
    INCLUDE_DIRS "src"
    REQUIRES esp_lvgl_port esp_lcd esp_lcd_touch driver mlib cmsis_core furi
)
