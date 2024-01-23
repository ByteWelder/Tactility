idf_component_register(
    SRC_DIRS "."
    INCLUDE_DIRS "."
    REQUIRES esp_lvgl_port esp_lcd esp_lcd_touch_gt911 driver
)

target_link_libraries(${COMPONENT_LIB} ${IDF_TARGET_NAME} tactility)