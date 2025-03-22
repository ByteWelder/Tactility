idf_component_register(
    SRC_DIRS "Source"
    INCLUDE_DIRS "Source" "/opt/esp/idf/components/esp_lcd/rgb/include/esp_lcd_panel_rgb.h"
    REQUIRES Tactility esp_lvgl_port esp_lcd
)
