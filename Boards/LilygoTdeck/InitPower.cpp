#include "config.h"
#include "TactilityCore.h"

#define TAG "tdeck"

static bool tdeck_power_on() {
    gpio_config_t device_power_signal_config = {
        .pin_bit_mask = BIT64(TDECK_POWERON_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    if (gpio_config(&device_power_signal_config) != ESP_OK) {
        return false;
    }

    if (gpio_set_level(TDECK_POWERON_GPIO, 1) != ESP_OK) {
        return false;
    }

    return true;
}

bool tdeck_init_power() {
    ESP_LOGI(TAG, "Power on");
    if (!tdeck_power_on()) {
        TT_LOG_E(TAG, "Power on failed");
        return false;
    }

    /**
     * Without this delay, the touch driver randomly fails when the device is USB-powered:
     *  > lcd_panel.io.i2c: panel_io_i2c_rx_buffer(135): i2c transaction failed
     *  > GT911: touch_gt911_read_cfg(352): GT911 read error!
     * This might not be a problem with a lipo, but I haven't been able to test that.
     * I tried to solve it just like I did with the keyboard:
     * By reading from I2C until it succeeds and to then init the driver.
     * It doesn't work, because it never recovers from the error.
     */
    TT_LOG_I(TAG, "Waiting after power-on");
    tt::delay_ms(TDECK_POWERON_DELAY);

    return true;
}
