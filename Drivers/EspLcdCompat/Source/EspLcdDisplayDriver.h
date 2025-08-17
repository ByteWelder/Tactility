#pragma once

#include "Tactility/Mutex.h"

#include <Tactility/hal/display/DisplayDriver.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lvgl_port_disp.h>

using namespace tt::hal;

class EspLcdDisplayDriver : public display::DisplayDriver {

    esp_lcd_panel_handle_t panelHandle;
    const lvgl_port_display_cfg_t& lvglPortDisplayConfig;
    std::shared_ptr<tt::Lock> lock;

public:

    EspLcdDisplayDriver(
        esp_lcd_panel_handle_t panelHandle,
        const lvgl_port_display_cfg_t& lvglPortDisplayConfig,
        std::shared_ptr<tt::Lock> lock
    ) : panelHandle(panelHandle), lvglPortDisplayConfig(lvglPortDisplayConfig), lock(lock) {}

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
        lock->lock();
        bool result = esp_lcd_panel_draw_bitmap(panelHandle, xStart, yStart, xEnd, yEnd, pixelData) == ESP_OK;
        lock->unlock();
        return result;
    }

    uint16_t getPixelWidth() const override { return lvglPortDisplayConfig.hres; }

    uint16_t getPixelHeight() const override { return lvglPortDisplayConfig.vres; }

    std::shared_ptr<tt::Lock> getLock() const { return lock; }
};
