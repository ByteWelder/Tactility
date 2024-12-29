#include "M5stackCore2.h"
#include "InitBoot.h"
#include "InitLvgl.h"
#include "hal/Core2Display.h"

extern const tt::hal::Configuration m5stack_core2 = {
    .initBoot = initBoot,
    .initLvgl = initLvgl,
    .createDisplay = createDisplay,
//    .sdcard = createM5SdCard(),
//    .power = m5stack_get_power,
    .i2c = {
        tt::hal::i2c::Configuration {
            .name = "Internal",
            .port = I2C_NUM_0,
            .initMode = tt::hal::i2c::InitByTactility,
            .canReinit = false, // Might be set to try after trying out what it does AXP and screen
            .hasMutableConfiguration = false,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_21,
                .scl_io_num = GPIO_NUM_22,
                .sda_pullup_en = true,
                .scl_pullup_en = true,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        },
        tt::hal::i2c::Configuration {
            .name = "External", // (Grove)
            .port = I2C_NUM_1,
            .initMode = tt::hal::i2c::InitByTactility,
            .canReinit = true,
            .hasMutableConfiguration = true,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_32,
                .scl_io_num = GPIO_NUM_33,
                .sda_pullup_en = true,
                .scl_pullup_en = true,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        }
    }
};
