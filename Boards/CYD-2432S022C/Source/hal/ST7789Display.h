#pragma once

#include <Tactility/hal/display/DisplayDevice.h>
#include "CYD2432S022CConstants.h"
#include <lvgl.h>
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
    std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() override { return config_->touch; }
    lv_display_t* getLvglDisplay() const override { return display_; }

    // Additional getters (not overrides)
    int getWidth() const { return config_->width; }
    int getHeight() const { return config_->height; }
    std::shared_ptr<tt::hal::touch::TouchDevice> getTouch() const { return config_->touch; }

private:
    void flush(const lv_area_t* area, unsigned char* color_p); // Updated signature
    void write_byte(uint8_t data);
    void set_address_window(int x, int y, int w, int h);

    std::unique_ptr<Configuration> config_;
    lv_display_t* display_ = nullptr;
};
