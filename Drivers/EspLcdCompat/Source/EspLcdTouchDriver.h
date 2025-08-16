#pragma once

#include <esp_lcd_touch.h>
#include <Tactility/hal/touch/TouchDriver.h>

class EspLcdTouchDriver final : public tt::hal::touch::TouchDriver {

    esp_lcd_touch_handle_t handle;

public:

    EspLcdTouchDriver(esp_lcd_touch_handle_t handle) : handle(handle) {}

    bool getTouchedPoints(uint16_t* x, uint16_t* y, uint16_t* strength, uint8_t* pointCount, uint8_t maxPointCount) override;
};
