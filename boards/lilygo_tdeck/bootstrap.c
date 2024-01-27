#include "config.h"
#include "keyboard.h"
#include "kernel.h"
#include "log.h"

#define TAG "tdeck_bootstrap"

lv_disp_t* lilygo_tdeck_init_display();

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

bool lilygo_tdeck_bootstrap() {
    if (!tdeck_power_on()) {
        TT_LOG_E(TAG, "failed to power on device");
    }

    // Give keyboard's ESP time to boot
    // It uses I2C and seems to interfere with the touch driver
    tt_delay_ms(500);

    if (!init_i2c()) {
        TT_LOG_E(TAG, "failed to init I2C");
    }

    keyboard_wait_for_response();

    return true;
}
