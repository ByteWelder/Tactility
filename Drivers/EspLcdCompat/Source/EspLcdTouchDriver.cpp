#include "EspLcdTouchDriver.h"

#include <Tactility/Logger.h>

static const auto LOGGER = tt::Logger("EspLcdTouchDriver");

bool EspLcdTouchDriver::getTouchedPoints(uint16_t* x, uint16_t* y, uint16_t* _Nullable strength, uint8_t* pointCount, uint8_t maxPointCount) {
    if (esp_lcd_touch_read_data(handle) != ESP_OK) {
        LOGGER.error("Read data failed");
        return false;
    }
    return esp_lcd_touch_get_coordinates(handle, x, y, strength, pointCount, maxPointCount);
}
