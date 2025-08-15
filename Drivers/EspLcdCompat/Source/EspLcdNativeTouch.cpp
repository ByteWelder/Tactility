#include "EspLcdNativeTouch.h"

bool EspLcdNativeTouch::getTouchedPoints(uint16_t* x, uint16_t* y, uint16_t* strength, uint8_t* pointCount, uint8_t maxPointCount) {
    return esp_lcd_touch_get_coordinates(handle, x, y, strength, pointCount, maxPointCount) == ESP_OK;
}
