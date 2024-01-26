idf_component_register(
    SRC_DIRS "."
    INCLUDE_DIRS "."
    REQUIRES lvgl esp_lcd esp_lcd_touch_gt911
)

target_link_libraries(${COMPONENT_LIB} ${IDF_TARGET_NAME} tactility)