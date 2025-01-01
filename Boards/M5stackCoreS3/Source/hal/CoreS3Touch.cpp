#include "CoreS3Touch.h"
#include "Log.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_lcd_touch_ft5x06.h"
#include "esp_lcd_touch.h"
#include "esp_lvgl_port.h"

#define TAG "cores3_touch"

bool CoreS3Touch::start(lv_display_t* display) {
    TT_LOG_I(TAG, "Starting");
    esp_lcd_panel_io_handle_t ioHandle;
    esp_lcd_touch_handle_t touchHandle;

    esp_lcd_panel_io_i2c_config_t touch_io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();

    // TODO: Check when ESP-IDF publishes fix (5.3.2 or 5.4.x)
    static_assert(ESP_IDF_VERSION == ESP_IDF_VERSION_VAL(5, 3, 1));
    esp_lcd_new_panel_io_i2c(I2C_NUM_0, &touch_io_config, &ioHandle);
    /*
    if (esp_lcd_new_panel_io_i2c(I2C_NUM_0, &touch_io_config, &ioHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Touch I2C IO init failed");
        return false;
    }
    */

    esp_lcd_touch_config_t config = {
        .x_max = 320,
        .y_max = 240,
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = GPIO_NUM_NC,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
        .process_coordinates = nullptr,
        .interrupt_callback = nullptr,
        .user_data = nullptr,
        .driver_data = nullptr
    };

    if (esp_lcd_touch_new_i2c_ft5x06(ioHandle, &config, &touchHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Driver init failed");
        return false;
    }

    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = display,
        .handle = touchHandle,
    };

    deviceHandle = lvgl_port_add_touch(&touch_cfg);
    if (deviceHandle == nullptr) {
        TT_LOG_E(TAG, "Adding touch failed");
        return false;
    }

    TT_LOG_I(TAG, "Finished");
    return true;
}

bool CoreS3Touch::stop() {
    tt_assert(deviceHandle != nullptr);
    lv_indev_delete(deviceHandle);
    deviceHandle = nullptr;
    return true;
}
