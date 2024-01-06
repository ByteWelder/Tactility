#include "tactility.h"

#include "esp_lcd_touch_gt911.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"

#define TOUCH_I2C_PORT 0

#define TAG "lilygo_tdeck_touch"

static bool create_touch_device(esp_lcd_panel_io_handle_t* io_handle, esp_lcd_touch_handle_t* touch_handle) {
    ESP_LOGI(TAG, "creating touch");

    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_18,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_io_num = GPIO_NUM_8,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = 400000
    };

    if (i2c_param_config(TOUCH_I2C_PORT, &i2c_conf) != ESP_OK) {
        ESP_LOGE(TAG, "i2c config failed");
        return false;
    }

    if (i2c_driver_install(TOUCH_I2C_PORT, i2c_conf.mode, 0, 0, 0) != ESP_OK) {
        ESP_LOGE(TAG, "i2c driver install failed");
        return false;
    }

    const esp_lcd_panel_io_i2c_config_t touch_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();

    if (esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)TOUCH_I2C_PORT, &touch_io_config, io_handle) != ESP_OK) {
        ESP_LOGE(TAG, "touch io i2c creation failed");
        return false;
    }

    ESP_LOGI(TAG, "create_touch");
    esp_lcd_touch_config_t config = {
        .x_max = 240,
        .y_max = 320,
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = GPIO_NUM_16,
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
        ESP_LOGE(TAG, "gt911 driver creation failed");
        return false;
    }

    return true;
}

TouchDriver lilygo_tdeck_touch_driver() {
    return (TouchDriver) {
        .name = "lilygo_tdeck_touch",
        .create_touch_device = &create_touch_device
    };
}
