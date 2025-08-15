#pragma once

#include <Tactility/hal/display/NativeDisplay.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lvgl_port_disp.h>

using namespace tt::hal;

class EspLcdNativeDisplay final : public display::NativeDisplay {

    esp_lcd_panel_handle_t panelHandle;
    const lvgl_port_display_cfg_t& lvglPortDisplayConfig;

public:
    EspLcdNativeDisplay(
        esp_lcd_panel_handle_t panelHandle,
        const lvgl_port_display_cfg_t& lvglPortDisplayConfig
    ) : panelHandle(panelHandle), lvglPortDisplayConfig(lvglPortDisplayConfig) {}

    display::ColorFormat getColorFormat() const override {
        using display::ColorFormat;
        switch (lvglPortDisplayConfig.color_format) {
            case LV_COLOR_FORMAT_I1:
                return ColorFormat::Monochrome;
            case LV_COLOR_FORMAT_RGB565:
                // swap_bytes is only used for the 565 color format
                // see lvgl_port_flush_callback() in esp_lvgl_port_disp.c
                return lvglPortDisplayConfig.flags.swap_bytes ? ColorFormat::BGR565 : ColorFormat::RGB565;
            case LV_COLOR_FORMAT_RGB888:
                return ColorFormat::RGB888;
            default:
                return ColorFormat::RGB565;
        }
    }

    bool drawBitmap(int xStart, int yStart, int xEnd, int yEnd, const void* pixelData) override {
        return esp_lcd_panel_draw_bitmap(panelHandle, xStart, yStart, xEnd, yEnd, pixelData) == ESP_OK;
    }

    uint16_t getPixelWidth() const override { return lvglPortDisplayConfig.hres; }
    uint16_t getPixelHeight() const override { return lvglPortDisplayConfig.vres; }
};
