#pragma once

#include <Tactility/hal/display/DisplayDevice.h>
#include <memory>
#include "CYD2432S022CConstants.h"

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();

class ST7789Display : public tt::hal::display::DisplayDevice {
public:
    struct Configuration {
        int width;
        int height;
        std::shared_ptr<tt::hal::touch::TouchDevice> touch;
    };

    ST7789Display(std::unique_ptr<Configuration> config);
    ~ST7789Display() override = default;

    void init() override;
    void flush(const lv_area_t *area, const lv_color_t *color_p) override;
    int get_width() const override { return config_->width; }
    int get_height() const override { return config_->height; }
    std::shared_ptr<tt::hal::touch::TouchDevice> get_touch() const override { return config_->touch; }

private:
    void write_byte(uint8_t data);
    void set_address_window(int x, int y, int w, int h);

    std::unique_ptr<Configuration> config_;
};
