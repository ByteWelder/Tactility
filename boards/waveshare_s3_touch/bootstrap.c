#include "config.h"
#include "tactility_core.h"
#include <driver/i2c.h>

#define TAG "waveshare_bootstrap"

#define WAVESHARE_I2C_MASTER_TX_BUF_DISABLE 0
#define WAVESHARE_I2C_MASTER_RX_BUF_DISABLE 0

static esp_err_t i2c_init(void) {
    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_8,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_io_num = GPIO_NUM_9,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = 400000
    };

    i2c_param_config(WAVESHARE_TOUCH_I2C_PORT, &i2c_conf);

    return i2c_driver_install(
        WAVESHARE_TOUCH_I2C_PORT,
        i2c_conf.mode,
        WAVESHARE_I2C_MASTER_RX_BUF_DISABLE,
        WAVESHARE_I2C_MASTER_TX_BUF_DISABLE,
        0
    ) == ESP_OK;
}

bool ws3t_bootstrap() {
    if (!i2c_init()) {
        TT_LOG_E(TAG, "I2C init failed");
        return false;
    }
    return true;
}
