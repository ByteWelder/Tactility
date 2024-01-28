#include "config.h"
#include "kernel.h"
#include "keyboard.h"
#include "log.h"
#include <driver/spi_common.h>

#define TAG "tdeck_bootstrap"

lv_disp_t* tdeck_display_init();

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

static bool init_spi() {
    spi_bus_config_t bus_config = {
        .sclk_io_num = TDECK_SPI_PIN_SCLK,
        .mosi_io_num = TDECK_SPI_PIN_MOSI,
        .miso_io_num = TDECK_SPI_PIN_MISO,
        .quadwp_io_num = -1, // Quad SPI LCD driver is not yet supported
        .quadhd_io_num = -1, // Quad SPI LCD driver is not yet supported
        .max_transfer_sz = TDECK_SPI_TRANSFER_SIZE_LIMIT,
    };

    if (spi_bus_initialize(TDECK_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO) != ESP_OK) {
        return false;
    } else {
        return true;
    }
}

bool tdeck_bootstrap() {
    ESP_LOGI(TAG, "Power on");
    if (!tdeck_power_on()) {
        TT_LOG_E(TAG, "Power on failed");
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
    tt_delay_ms(TDECK_POWERON_DELAY);

    TT_LOG_I(TAG, "Init I2C");
    if (!init_i2c()) {
        TT_LOG_E(TAG, "Init I2C failed");
        return false;
    }

    TT_LOG_I(TAG, "Init SPI");
    if (!init_spi()) {
        TT_LOG_E(TAG, "Init SPI failed");
        return false;
    }

    keyboard_wait_for_response();

    return true;
}
