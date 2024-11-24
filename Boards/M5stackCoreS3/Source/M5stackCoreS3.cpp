#include "M5stackCoreS3.h"
#include "M5stackShared.h"

extern const tt::hal::sdcard::SdCard m5stack_cores3_sdcard;

const tt::hal::Configuration m5stack_cores3 = {
    .initPower = &m5stack_bootstrap,
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
                .sda_io_num = GPIO_NUM_12,
                .scl_io_num = GPIO_NUM_11,
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
                .sda_io_num = GPIO_NUM_2,
                .scl_io_num = GPIO_NUM_1,
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
