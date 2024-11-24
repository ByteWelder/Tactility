#include "M5stackCore2.h"
#include "M5stackShared.h"
#include <driver/spi_common.h>

#define TAG "test"

static bool init_spi() {
    spi_bus_config_t bus_config = {
        .mosi_io_num = GPIO_NUM_23,
        .miso_io_num = GPIO_NUM_38,
        .sclk_io_num = GPIO_NUM_18,
        .quadwp_io_num = -1, // Quad SPI LCD driver is not yet supported
        .quadhd_io_num = -1, // Quad SPI LCD driver is not yet supported
        .max_transfer_sz = 64000,
    };

    return spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO) == ESP_OK;
}

bool init_hardware() {

    TT_LOG_I(TAG, "Init SPI");
    if (!init_spi()) {
        TT_LOG_E(TAG, "Init SPI failed");
        return false;
    }

    return true;

}
extern const tt::hal::Configuration m5stack_core2 = {
    .initPower = &m5stack_bootstrap,
    .initHardware = &init_hardware,
    .initLvgl = &m5stack_lvgl_init,
    .sdcard = &m5stack_sdcard,
    .power = &m5stack_power,
    .i2c = {
        // Internal
        tt::hal::i2c::Configuration {
            .port = I2C_NUM_0,
            .initMode = tt::hal::i2c::InitByExternal,
            .canReinit = false,
            .hasMutableConfiguration = false,
            .timeout = 1000,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_21,
                .scl_io_num = GPIO_NUM_22,
                .sda_pullup_en = GPIO_PULLUP_ENABLE,
                .scl_pullup_en = GPIO_PULLUP_ENABLE,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        },
        // External (Grove)
        tt::hal::i2c::Configuration {
            .port = I2C_NUM_1,
            .initMode = tt::hal::i2c::InitByExternal,
            .canReinit = true,
            .hasMutableConfiguration = true,
            .timeout = 1000,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_32,
                .scl_io_num = GPIO_NUM_33,
                .sda_pullup_en = GPIO_PULLUP_ENABLE,
                .scl_pullup_en = GPIO_PULLUP_ENABLE,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        }
    }
};
