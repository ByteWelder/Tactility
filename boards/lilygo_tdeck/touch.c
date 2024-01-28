#include "config.h"
#include "esp_err.h"
#include "esp_lcd_panel_io_interface.h"
#include "esp_lcd_touch_gt911.h"
#include "log.h"
#include <kernel.h>

#define TAG "tdeck_touch"

bool tdeck_init_touch(esp_lcd_panel_io_handle_t* io_handle, esp_lcd_touch_handle_t* touch_handle) {
    const esp_lcd_panel_io_i2c_config_t touch_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    if (
        esp_lcd_new_panel_io_i2c(
            (esp_lcd_i2c_bus_handle_t)TDECK_TOUCH_I2C_BUS_HANDLE,
            &touch_io_config,
            io_handle
        ) != ESP_OK
    ) {
        TT_LOG_E(TAG, "touch io i2c creation failed");
        return false;
    }

    esp_lcd_touch_config_t config = {
        .x_max = TDECK_TOUCH_X_MAX,
        .y_max = TDECK_TOUCH_Y_MAX,
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = TDECK_TOUCH_PIN_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 1,
            .mirror_x = 1,
            .mirror_y = 0,
        },
        .process_coordinates = NULL,
        .interrupt_callback = NULL,
        .user_data = NULL
    };

    if (esp_lcd_touch_new_i2c_gt911(*io_handle, &config, touch_handle) != ESP_OK) {
        TT_LOG_E(TAG, "gt911 driver creation failed");
        return false;
    }

    return true;
}