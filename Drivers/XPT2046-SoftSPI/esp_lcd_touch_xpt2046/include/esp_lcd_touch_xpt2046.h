#pragma once

#include <memory>
#include <esp_lcd_touch.h>
#include <lvgl.h>
#include "SoftSPI.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_LCD_TOUCH_SOFTSPI_CLOCK_HZ (500 * 1000)

// Extend ESP-IDF's config with calibration and orientation
typedef struct {
    esp_lcd_touch_config_t base;
    uint16_t x_min_raw;
    uint16_t x_max_raw;
    uint16_t y_min_raw;
    uint16_t y_max_raw;
    bool swap_xy;
    bool mirror_x;
    bool mirror_y;
} esp_lcd_touch_xpt2046_config_t;

class XPT2046_SoftSPI {
public:
    struct Config {
        gpio_num_t cs_pin;
        gpio_num_t int_pin;
        gpio_num_t miso_pin;
        gpio_num_t mosi_pin;
        gpio_num_t sck_pin;
        esp_lcd_touch_xpt2046_config_t touch_config;
    };

    static std::unique_ptr<XPT2046_SoftSPI> create(const Config& config);
    ~XPT2046_SoftSPI();

    esp_lcd_touch_handle_t get_handle() const { return handle_; }
    lv_indev_t* get_lvgl_indev() const { return indev_; }
    void get_raw_touch(uint16_t& x, uint16_t& y);

private:
    XPT2046_SoftSPI(const Config& config);
    bool init();
    static esp_err_t read_data(esp_lcd_touch_handle_t tp);
    static bool get_xy(esp_lcd_touch_handle_t tp, uint16_t* x, uint16_t* y,
                       uint16_t* strength, uint8_t* point_num, uint8_t max_point_num);
    static esp_err_t del(esp_lcd_touch_handle_t tp);
    static void lvgl_read_cb(lv_indev_t* indev, lv_indev_data_t* data);
    esp_err_t read_register(uint8_t reg, uint16_t* value);

    esp_lcd_touch_handle_t handle_;
    lv_indev_t* indev_;
    std::unique_ptr<SoftSPI> spi_;
    gpio_num_t cs_pin_;
};

#ifdef __cplusplus
}
#endif
