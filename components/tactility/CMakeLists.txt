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
        tactility-core
        mlib
        nvs_flash
        spiffs
)

set(ASSETS_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
spiffs_create_partition_image(assets ${ASSETS_SRC_DIR} FLASH_IN_PROJECT)

set(CONFIG_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/config")
spiffs_create_partition_image(config ${CONFIG_SRC_DIR} FLASH_IN_PROJECT)
