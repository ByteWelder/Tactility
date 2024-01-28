#include "config.h"
#include "driver/i2c.h"
#include "log.h"
#include "esp_err.h"
#include "esp_lcd_touch_cst816s.h"

#define TAG "twodotfour_cst816"

bool twodotfour_touch_init(esp_lcd_panel_io_handle_t* io_handle, esp_lcd_touch_handle_t* touch_handle) {
    TT_LOG_I(TAG, "Touch init");

    const esp_lcd_panel_io_i2c_config_t touch_io_config = ESP_LCD_TOUCH_IO_I2C_CST816S_CONFIG();

    if (esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)TWODOTFOUR_TOUCH_I2C_PORT, &touch_io_config, io_handle) != ESP_OK) {
        TT_LOG_E(TAG, "Touch I2C IO init failed");
        return false;
    }

    esp_lcd_touch_config_t config = {
        .x_max = 240,
        .y_max = 320,
        .rst_gpio_num = GPIO_NUM_25,
        .int_gpio_num = GPIO_NUM_21,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
        .process_coordinates = NULL,
        .interrupt_callback = NULL,
        .user_data = NULL
    };

    if (esp_lcd_touch_new_i2c_cst816s(*io_handle, &config, touch_handle) != ESP_OK) {
        TT_LOG_E(TAG, "Driver init failed");
        return false;
    }

    return true;
}
