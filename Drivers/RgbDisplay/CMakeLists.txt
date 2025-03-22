idf_component_register(
    SRC_DIRS "Source"
    INCLUDE_DIRS "Source" "/opt/esp/idf/components/esp_lcd/rgb/include/"
    REQUIRES Tactility esp_lvgl_port esp_lcd
)
