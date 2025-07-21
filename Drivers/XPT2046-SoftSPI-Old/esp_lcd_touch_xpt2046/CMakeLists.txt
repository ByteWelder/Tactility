idf_component_register(SRCS "esp_lcd_touch_xpt2046.cpp" "SoftSPI.h"
                       INCLUDE_DIRS "include"
                       REQUIRES "driver esp_lcd_touch esp_timer")
