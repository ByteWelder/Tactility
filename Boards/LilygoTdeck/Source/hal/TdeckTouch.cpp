#include "TdeckTouch.h"

#include "esp_err.h"
#include "esp_lcd_touch_gt911.h"
#include "Log.h"
#include "kernel/Kernel.h"
#include "esp_lvgl_port.h"

#define TAG "tdeck_touch"

// Touch (GT911)
#define TDECK_TOUCH_I2C_BUS_HANDLE I2C_NUM_0
#define TDECK_TOUCH_X_MAX 240
#define TDECK_TOUCH_Y_MAX 320

bool TdeckTouch::start(lv_display_t* display) {
    const esp_lcd_panel_io_i2c_config_t io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();

    // TODO: Revert on new ESP-IDF version
    static_assert(ESP_IDF_VERSION == ESP_IDF_VERSION_VAL(5, 3, 1));
    esp_lcd_new_panel_io_i2c(
        (esp_lcd_i2c_bus_handle_t)TDECK_TOUCH_I2C_BUS_HANDLE,
        &io_config,
        &ioHandle
    );
    /*
    if (
        esp_lcd_new_panel_io_i2c(
            (esp_lcd_i2c_bus_handle_t)TDECK_TOUCH_I2C_BUS_HANDLE,
            &touch_io_config,
            &ioHandle
        ) != ESP_OK
    ) {
        TT_LOG_E(TAG, "touch io i2c creation failed");
        return false;
    }
    */

    esp_lcd_touch_config_t config = {
        .x_max = TDECK_TOUCH_X_MAX,
        .y_max = TDECK_TOUCH_Y_MAX,
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = GPIO_NUM_NC, // There is no reset pin for touch on the T-Deck, leading to a bug in esp_lvgl_port firing multiple click events when tapping the screen
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 1,
            .mirror_x = 1,
            .mirror_y = 0,
        },
        .process_coordinates = nullptr,
        .interrupt_callback = nullptr,
        .user_data = nullptr,
        .driver_data = nullptr
    };

    if (esp_lcd_touch_new_i2c_gt911(ioHandle, &config, &touchHandle) != ESP_OK) {
        TT_LOG_E(TAG, "GT199 driver init failed");
        // TODO: De-init IO
        return false;
    }

    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = display,
        .handle = touchHandle,
    };

    TT_LOG_I(TAG, "Adding touch to LVGL");
    deviceHandle = lvgl_port_add_touch(&touch_cfg);
    if (deviceHandle == nullptr) {
        TT_LOG_E(TAG, "Adding touch failed");
        return false;
    }

    return true;
}

bool TdeckTouch::stop() {
    if (esp_lcd_touch_del(touchHandle) == ESP_OK) {
        touchHandle = nullptr;
    } else {
        TT_LOG_E(TAG, "Deleting driver failed");
        return false;
    }

    if (esp_lcd_panel_io_del(ioHandle) == ESP_OK) {
        ioHandle = nullptr;
    } else {
        TT_LOG_E(TAG, "Deleting IO handle failed");
        return false;
    }

    return true;
}
