#include "Config.h"
#include "TactilityCore.h"
#include "hal/YellowTouchConstants.h"
#include <driver/spi_common.h>

#define TAG "twodotfour_bootstrap"

static bool init_i2c() {
    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_33,
        .scl_io_num = GPIO_NUM_32,
        .sda_pullup_en = false,
        .scl_pullup_en = false,
        .master = {
            .clk_speed = 400000
        }
    };

    if (i2c_param_config(TWODOTFOUR_TOUCH_I2C_PORT, &i2c_conf) != ESP_OK) {
        TT_LOG_E(TAG, "i2c config failed");
        return false;
    }

    if (i2c_driver_install(TWODOTFOUR_TOUCH_I2C_PORT, i2c_conf.mode, 0, 0, 0) != ESP_OK) {
        TT_LOG_E(TAG, "i2c driver install failed");
        return false;
    }

    return true;
}

static bool init_spi2() {
     const spi_bus_config_t bus_config = {
         .mosi_io_num = TWODOTFOUR_SPI2_PIN_MOSI,
         .miso_io_num = GPIO_NUM_NC,
         .sclk_io_num = TWODOTFOUR_SPI2_PIN_SCLK,
         .quadwp_io_num = GPIO_NUM_NC,
         .quadhd_io_num = GPIO_NUM_NC,
         .max_transfer_sz = TWODOTFOUR_SPI2_TRANSACTION_LIMIT
    };

     if (spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO) != ESP_OK) {
         TT_LOG_E(TAG, "SPI bus init failed");
         return false;
     }

     return true;
}

static bool init_spi3() {
    const spi_bus_config_t bus_config = {
        .mosi_io_num = TWODOTFOUR_SPI3_PIN_MOSI,
        .miso_io_num = TWODOTFOUR_SPI3_PIN_MISO,
        .sclk_io_num = TWODOTFOUR_SPI3_PIN_SCLK,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = TWODOTFOUR_SPI3_TRANSACTION_LIMIT
    };

    if (spi_bus_initialize(SPI3_HOST, &bus_config, SPI_DMA_CH_AUTO) != ESP_OK) {
        TT_LOG_E(TAG, "SPI bus init failed");
        return false;
    }

    return true;
}

bool twodotfour_boot() {
    TT_LOG_I(TAG, "Init I2C");
    if (!init_i2c()) {
        TT_LOG_E(TAG, "Init I2C failed");
        return false;
    }

    TT_LOG_I(TAG, "Init SPI2");
    if (!init_spi2()) {
        TT_LOG_E(TAG, "Init SPI2 failed");
        return false;
    }

    TT_LOG_I(TAG, "Init SPI3");
    if (!init_spi3()) {
        TT_LOG_E(TAG, "Init SPI3 failed");
        return false;
    }

    return true;
}
