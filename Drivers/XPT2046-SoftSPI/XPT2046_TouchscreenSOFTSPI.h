#pragma once

#include "esp_lcd_touch_xpt2046.h"

class XPT2046_SoftSPI_Wrapper {
public:
    struct Config {
        gpio_num_t cs_pin;
        gpio_num_t int_pin;
        gpio_num_t miso_pin;
        gpio_num_t mosi_pin;
        gpio_num_t sck_pin;
        uint16_t x_max;
        uint16_t y_max;
        bool swap_xy;
        bool mirror_x;
        bool mirror_y;
        uint16_t x_min_raw;
        uint16_t x_max_raw;
        uint16_t y_min_raw;
        uint16_t y_max_raw;
    };

    static std::unique_ptr<XPT2046_SoftSPI_Wrapper> create(const Config& config);
    ~XPT2046_SoftSPI_Wrapper() = default;

    esp_lcd_touch_handle_t get_touch_handle() const { return driver_->get_handle(); }
    lv_indev_t* get_lvgl_indev() const { return driver_->get_lvgl_indev(); }

private:
    XPT2046_SoftSPI_Wrapper(std::unique_ptr<XPT2046_SoftSPI> driver) : driver_(std::move(driver)) {}
    std::unique_ptr<XPT2046_SoftSPI> driver_;
};
