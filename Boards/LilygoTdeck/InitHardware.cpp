#include "TactilityCore.h"
#include "config.h"
#include "display.h"
#include "keyboard.h"
#include <driver/spi_common.h>

#define TAG "tdeck"

static bool init_i2c() {
    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_18,
        .scl_io_num = GPIO_NUM_8,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master = {
            .clk_speed = 400000
        }
    };

    return i2c_param_config(TDECK_I2C_BUS_HANDLE, &i2c_conf) == ESP_OK && i2c_driver_install(TDECK_I2C_BUS_HANDLE, i2c_conf.mode, 0, 0, 0) == ESP_OK;
}

static bool init_spi() {
    spi_bus_config_t bus_config = {
        .mosi_io_num = TDECK_SPI_PIN_MOSI,
        .miso_io_num = TDECK_SPI_PIN_MISO,
        .sclk_io_num = TDECK_SPI_PIN_SCLK,
        .quadwp_io_num = -1, // Quad SPI LCD driver is not yet supported
        .quadhd_io_num = -1, // Quad SPI LCD driver is not yet supported
        .max_transfer_sz = TDECK_SPI_TRANSFER_SIZE_LIMIT,
    };

    return spi_bus_initialize(TDECK_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO) == ESP_OK;
}

bool tdeck_init_hardware() {
    TT_LOG_I(TAG, "Init SPI");
    if (!init_spi()) {
        TT_LOG_E(TAG, "Init SPI failed");
        return false;
    }

    // Don't turn the backlight on yet - Tactility init will take care of it
    TT_LOG_I(TAG, "Init backlight");
    if (!tdeck_backlight_init()) {
        TT_LOG_E(TAG, "Init backlight failed");
        return false;
    }

    // We wait for the keyboard peripheral to be booted up
    keyboard_wait_for_response();

    return true;
}