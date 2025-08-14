#pragma once

#include <Tactility/hal/display/NativeDisplay.h>
#include <esp_lcd_panel_ops.h>

namespace tt::hal::display {

class EspLcdNativeDisplay final : public NativeDisplay {

    esp_lcd_panel_handle_t panelHandle;
    ColorFormat colorFormat;
    uint16_t pixelWidth;
    uint16_t pixelHeight;

public:
    EspLcdNativeDisplay(
        esp_lcd_panel_handle_t panelHandle,
        ColorFormat colorFormat,
        uint16_t pixelWidth,
        uint16_t pixelHeight
    ) : panelHandle(panelHandle), colorFormat(colorFormat), pixelWidth(pixelWidth), pixelHeight(pixelHeight) {}

    ColorFormat getColorFormat() const override { return colorFormat; }

    bool drawBitmap(int xStart, int yStart, int xEnd, int yEnd, const void* pixelData) override {
        return esp_lcd_panel_draw_bitmap(panelHandle, xStart, yStart, xEnd, yEnd, pixelData) == ESP_OK;
    }

    uint16_t getPixelWidth() const override { return pixelWidth; }
    uint16_t getPixelHeight() const override { return pixelHeight; }
};

}
