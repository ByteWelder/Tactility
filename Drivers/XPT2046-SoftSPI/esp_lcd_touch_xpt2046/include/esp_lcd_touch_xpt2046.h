#pragma once

#include <memory>
#include <esp_lcd_touch.h>
#include <lvgl.h>
#include "SoftSPI.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_LCD_TOUCH_SPI_CLOCK_HZ (500 * 1000)  // Slower for SoftSPI stability

struct esp_lcd_touch_config_t {
    gpio_num_t int_gpio_num;
    uint16_t x_max;
    uint16_t y_max;
    bool swap_xy;
    bool mirror_x;
    bool mirror_y;
    uint16_t x_min_raw;  // Calibration
    uint16_t x_max_raw;
    uint16_t y_min_raw;
    uint16_t y_max_raw;
    esp_lcd_touch_interrupt_callback_t interrupt_callback;
    struct {
        bool interrupt_level;
    } levels;
};

class XPT2046_SoftSPI {
public:
    struct Config {
        gpio_num_t cs_pin;
        gpio_num_t int_pin;
        gpio_num_t miso_pin;
        gpio_num_t mosi_pin;
        gpio_num_t sck_pin;
        esp_lcd_touch_config_t touch_config;
    };

    static std::unique_ptr<XPT2046_SoftSPI> create(const Config& config);
    ~XPT2046_SoftSPI();

    esp_lcd_touch_handle_t get_handle() const { return handle_; }
    lv_indev_t* get_lvgl_indev() const { return indev_; }

private:
    XPT2046_SoftSPI(gpio_num_t cs_pin, const esp_lcd_touch_config_t& config);
    bool init();
    static esp_err_t read_data(esp_lcd_touch_handle_t tp);
    static bool get_xy(esp_lcd_touch_handle_t tp, uint16_t* x, uint16_t* y,
                       uint16_t* strength, uint8_t* point_num, uint8_t max_point_num);
    static esp_err_t del(esp_lcd_touch_handle_t tp);
    static void lvgl_read_cb(lv_indev_t* indev, lv_indev_data_t* data);

    esp_lcd_touch_handle_t handle_;
    lv_indev_t* indev_;
    std::unique_ptr<SoftSPI> spi_;
};

#ifdef __cplusplus
}
#endif
