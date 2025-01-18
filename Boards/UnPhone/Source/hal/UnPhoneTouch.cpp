#include "UnPhoneTouch.h"

#include "esp_err.h"
#include "Log.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_touch_xpt2046.h"

#define TAG "unphone_touch"

#define UNPHONE_TOUCH_X_MAX 320
#define UNPHONE_TOUCH_Y_MAX 480

bool UnPhoneTouch::start(lv_display_t* display) {
    const esp_lcd_panel_io_spi_config_t io_config = ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(GPIO_NUM_38);

    if (esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &ioHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Touch IO SPI creation failed");
        return false;
    }

    esp_lcd_touch_config_t config = {
        .x_max = UNPHONE_TOUCH_X_MAX,
        .y_max = UNPHONE_TOUCH_Y_MAX,
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = GPIO_NUM_NC, // There is no reset pin for touch on the T-Deck, leading to a bug in esp_lvgl_port firing multiple click events when tapping the screen
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

    if (esp_lcd_touch_new_spi_xpt2046(ioHandle, &config, &touchHandle) != ESP_OK) {
        TT_LOG_E(TAG, "XPT2046 driver init failed");
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

bool UnPhoneTouch::stop() {
    cleanup();
    return true;
}

void UnPhoneTouch::cleanup() {
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
