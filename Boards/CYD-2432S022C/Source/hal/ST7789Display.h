#pragma once

#include <Tactility/hal/display/DisplayDevice.h>
#include "CYD2432S022CConstants.h"
#include <lvgl.h> // Add LVGL header for lv_display_t
#include <memory>
#include <string>

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

    // Methods from Device
    std::string getName() const override { return "ST7789 Display"; }
    std::string getDescription() const override { return "ST7789 240x320 Display with 8-bit Parallel Interface"; }

    // Methods from DisplayDevice
    bool start() override;
    bool stop() override;
    void flush(lv_display_t* disp, const lv_area_t* area, uint8_t* color_p) override;
    std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() override { return config_->touch; }
    lv_display_t* getLvglDisplay() const override { return display_; }
    int get_width() const override { return config_->width; }
    int get_height() const override { return config_->height; }
    std::shared_ptr<tt::hal::touch::TouchDevice> get_touch() const override { return config_->touch; }

private:
    void write_byte(uint8_t data);
    void set_address_window(int x, int y, int w, int h);

    std::unique_ptr<Configuration> config_;
    lv_display_t* display_ = nullptr; // Store the LVGL display
};
