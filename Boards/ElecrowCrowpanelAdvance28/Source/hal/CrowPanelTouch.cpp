#include "CrowPanelTouch.h"
#include "esp_lcd_touch_ft5x06.h"

#include <Tactility/Log.h>
#include <esp_err.h>
#include <esp_lvgl_port.h>

#define TAG "crowpanel_touch"

#define CROWPANEL_TOUCH_I2C_BUS_HANDLE I2C_NUM_0
#define CROWPANEL_TOUCH_X_MAX 240
#define CROWPANEL_TOUCH_Y_MAX 320

bool CrowPanelTouch::start(lv_display_t* display) {
    esp_lcd_panel_io_i2c_config_t io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();

    if (esp_lcd_new_panel_io_i2c(CROWPANEL_TOUCH_I2C_BUS_HANDLE, &io_config, &ioHandle) != ESP_OK) {
        TT_LOG_E(TAG, "touch io i2c creation failed");
        return false;
    }

    esp_lcd_touch_config_t config = {
        .x_max = CROWPANEL_TOUCH_X_MAX,
        .y_max = CROWPANEL_TOUCH_Y_MAX,
        .rst_gpio_num = GPIO_NUM_NC, // GPIO_NUM_48,
        .int_gpio_num = GPIO_NUM_NC, // GPIO_NUM_47,
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

    if (esp_lcd_touch_new_i2c_ft5x06(ioHandle, &config, &touchHandle) != ESP_OK) {
        TT_LOG_E(TAG, "GT911 driver init failed");
        cleanup();
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
        cleanup();
        return false;
    }

    return true;
}

bool CrowPanelTouch::stop() {
    cleanup();
    return true;
}

void CrowPanelTouch::cleanup() {
    if (deviceHandle != nullptr) {
        lv_indev_delete(deviceHandle);
        deviceHandle = nullptr;
    }

    if (touchHandle != nullptr) {
        esp_lcd_touch_del(touchHandle);
        touchHandle = nullptr;
    }

    if (ioHandle != nullptr) {
        esp_lcd_panel_io_del(ioHandle);
        ioHandle = nullptr;
    }
}
