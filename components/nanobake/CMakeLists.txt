idf_component_register(
    SRC_DIRS "src"
    "src/apps/system/system_info"
    "src/apps/services/desktop"
    "src/apps/services/loader"
    "src/apps/services/gui"
    INCLUDE_DIRS "src"
    REQUIRES esp_lvgl_port esp_lcd esp_lcd_touch driver mlib cmsis_core furi nvs_flash spiffs fatfs
)

set(ASSETS_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
spiffs_create_partition_image(assets ${ASSETS_SRC_DIR} FLASH_IN_PROJECT)

set(CONFIG_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/config")
spiffs_create_partition_image(config ${CONFIG_SRC_DIR} FLASH_IN_PROJECT)
