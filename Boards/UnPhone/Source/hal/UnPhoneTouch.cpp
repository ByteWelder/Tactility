#include "UnPhoneTouch.h"

#include "Log.h"
#include "esp_err.h"
#include "esp_lcd_touch_xpt2046.h"
#include "esp_lvgl_port.h"
#include "lvgl/LvglSync.h"

#define TAG "unphone_touch"

#define UNPHONE_TOUCH_X_MAX 320
#define UNPHONE_TOUCH_Y_MAX 480

UnPhoneTouch* UnPhoneTouch::instance = nullptr;

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

    instance = this;
    return true;
}

bool UnPhoneTouch::stop() {
    instance = nullptr;
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

bool UnPhoneTouch::getVBat(float& outputVbat) {
    if (touchHandle != nullptr) {
        // Shares the SPI bus with the display, so we have to sync/lock as this method might be called from anywhere
        if (tt::lvgl::lock(50 / portTICK_PERIOD_MS)) {
            esp_lcd_touch_xpt2046_read_battery_level(touchHandle, &outputVbat);
            tt::lvgl::unlock();
            return true;
        }
    }
    return false;
}
