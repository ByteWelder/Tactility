idf_component_register(
    SRC_DIRS "."
    INCLUDE_DIRS "."
    REQUIRES esp_lvgl_port esp_lcd_touch_cst816s esp_lcd_ili9341 driver vfs fatfs
)

target_link_libraries(${COMPONENT_LIB} ${IDF_TARGET_NAME} tactility)