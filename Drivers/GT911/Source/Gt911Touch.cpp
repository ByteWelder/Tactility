#include "Gt911Touch.h"

#include <Tactility/Log.h>
#include <Tactility/hal/i2c/I2c.h>

#include <esp_lcd_touch_gt911.h>
#include <esp_err.h>
#include <esp_lvgl_port.h>

#define TAG "GT911"

bool Gt911Touch::start(lv_display_t* display) {
    esp_lcd_panel_io_i2c_config_t io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();

    /**
     * When the interrupt pin is low, the address is 0x5D. Otherwise it is 0x14.
     * There is not reset pin, and the current driver fails when you only specify the interrupt pin.
     * Because of that, we don't use the interrupt pin and we'll simply scan the bus instead:
     */
    if (tt::hal::i2c::masterHasDeviceAtAddress(configuration->port, ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS)) {
        io_config.dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS;
    } else if (tt::hal::i2c::masterHasDeviceAtAddress(configuration->port, ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP)) {
        io_config.dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP;
    } else {
        TT_LOG_E(TAG, "No device found on I2C bus");
        return false;
    }

    if (esp_lcd_new_panel_io_i2c(configuration->port, &io_config, &ioHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Touch IO I2C creation failed");
        return false;
    }

    esp_lcd_touch_config_t config = {
        .x_max = configuration->xMax,
        .y_max = configuration->yMax,
        .rst_gpio_num = configuration->pinReset,
        .int_gpio_num = configuration->pinInterrupt,
        .levels = {
            .reset = configuration->pinResetLevel,
            .interrupt = configuration->pinInterruptLevel,
        },
        .flags = {
            .swap_xy = configuration->swapXy,
            .mirror_x = configuration->mirrorX,
            .mirror_y = configuration->mirrorY,
        },
        .process_coordinates = nullptr,
        .interrupt_callback = nullptr,
        .user_data = nullptr,
        .driver_data = nullptr
    };

    if (esp_lcd_touch_new_i2c_gt911(ioHandle, &config, &touchHandle) != ESP_OK) {
        TT_LOG_E(TAG, "Driver init failed");
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

bool Gt911Touch::stop() {
    cleanup();
    return true;
}

void Gt911Touch::cleanup() {
    if (deviceHandle != nullptr) {
        lv_indev_delete(deviceHandle);
        deviceHandle = nullptr;
        touchHandle = nullptr;
    }

    if (ioHandle != nullptr) {
        esp_lcd_panel_io_del(ioHandle);
        ioHandle = nullptr;
    }
}
