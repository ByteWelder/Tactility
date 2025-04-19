idf_component_register(SRCS "esp_lcd_touch_xpt2046.cpp"
                       INCLUDE_DIRS "include" "SoftSPI.h"
                       REQUIRES "driver esp_lcd_touch esp_timer")
