#pragma once

#include <esp_lcd_touch.h>
#include <Tactility/hal/touch/NativeTouch.h>

class EspLcdNativeTouch final : public tt::hal::touch::NativeTouch {

    esp_lcd_touch_handle_t handle;

public:

    EspLcdNativeTouch(esp_lcd_touch_handle_t handle) : handle(handle) {}

    bool getTouchedPoints(uint16_t* x, uint16_t* y, uint16_t* strength, uint8_t* pointCount, uint8_t maxPointCount) override;
};
