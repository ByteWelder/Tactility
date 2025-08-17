#pragma once

#include <Tactility/Mutex.h>
#include <Tactility/hal/display/DisplayDriver.h>

#include <esp_lcd_panel_ops.h>

class EspLcdDisplayDriver : public tt::hal::display::DisplayDriver {

    esp_lcd_panel_handle_t panelHandle;
    std::shared_ptr<tt::Lock> lock;
    uint16_t hRes;
    uint16_t vRes;
    tt::hal::display::ColorFormat colorFormat;

public:

    EspLcdDisplayDriver(
        esp_lcd_panel_handle_t panelHandle,
        std::shared_ptr<tt::Lock> lock,
        uint16_t hRes,
        uint16_t vRes,
        tt::hal::display::ColorFormat colorFormat
    ) : panelHandle(panelHandle), lock(lock), hRes(hRes), vRes(vRes), colorFormat(colorFormat) {}

    tt::hal::display::ColorFormat getColorFormat() const override {
        return colorFormat;
    }

    bool drawBitmap(int xStart, int yStart, int xEnd, int yEnd, const void* pixelData) override {
        bool result = esp_lcd_panel_draw_bitmap(panelHandle, xStart, yStart, xEnd, yEnd, pixelData) == ESP_OK;
        return result;
    }

    uint16_t getPixelWidth() const override { return hRes; }

    uint16_t getPixelHeight() const override { return vRes; }

    std::shared_ptr<tt::Lock> getLock() const override { return lock; }
};
