#pragma once

#include <memory>
#include <esp_lcd_touch.h>
#include <lvgl.h>


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

class XPT2046_SoftSPI;

    uint32_t spi_delay_us = 10; ///< Delay between SPI signal transitions (default 10us)
    uint32_t spi_post_command_delay_us = 2; ///< Delay after sending command before reading (default 2us)
};

    static std::unique_ptr<XPT2046_SoftSPI> create(const Config& config);
    ~XPT2046_SoftSPI();

    esp_lcd_touch_handle_t get_handle() const { return handle_; }
    /**
     * @brief Get the LVGL input device for this touchscreen
     * 
     * @return lv_indev_t* LVGL input device pointer
     */
    lv_indev_t* get_lvgl_indev() const { return indev_; }

    /**
     * @brief Self-test method for communication verification
     *
     * @return true if communication is successful, false otherwise
     */
    bool self_test();
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
    gpio_num_t cs_pin_;
    gpio_num_t int_pin_;
    std::unique_ptr<SoftSPI> spi_;
};

#ifdef __cplusplus
}
#endif
