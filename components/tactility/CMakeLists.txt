idf_component_register(
    SRC_DIRS "src"
    "src/apps/desktop"
    "src/apps/system/system_info"
    "src/apps/system/wifi_connect"
    "src/apps/system/wifi_manage"
    "src/services/loader"
    "src/services/gui"
    "src/services/gui/widgets"
    "src/services/wifi"
    "src/ui"

    INCLUDE_DIRS "src"

    REQUIRES
        esp_lcd
        esp_lcd_touch
        esp_lvgl_port
        esp_wifi
        driver
        fatfs
        nvs_flash
        spiffs
)

target_link_libraries(${COMPONENT_LIB} ${IDF_TARGET_NAME} tactility-core)
