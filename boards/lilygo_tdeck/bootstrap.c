#include "config.h"
#include "keyboard.h"
#include "kernel.h"
#include "log.h"

#define TAG "tdeck_bootstrap"

lv_disp_t* tdeck_init_display();

static bool tdeck_power_on() {
    ESP_LOGI(TAG, "power on");
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

static bool init_i2c() {
    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_18,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_io_num = GPIO_NUM_8,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = 400000
    };

    return i2c_param_config(TDECK_I2C_BUS_HANDLE, &i2c_conf) == ESP_OK
        && i2c_driver_install(TDECK_I2C_BUS_HANDLE, i2c_conf.mode, 0, 0, 0) == ESP_OK;
}

bool tdeck_bootstrap() {
    if (!tdeck_power_on()) {
        TT_LOG_E(TAG, "failed to power on device");
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
    TT_LOG_I(TAG, "waiting after power-on");
    tt_delay_ms(2000);

    if (!init_i2c()) {
        TT_LOG_E(TAG, "failed to init I2C");
    }

    keyboard_wait_for_response();

    return true;
}
