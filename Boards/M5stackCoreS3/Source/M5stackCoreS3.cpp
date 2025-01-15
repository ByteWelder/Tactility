#include "M5stackCoreS3.h"
#include "InitBoot.h"
#include "InitLvgl.h"
#include "hal/CoreS3Display.h"
#include "hal/CoreS3SdCard.h"
#include "hal/CoreS3Power.h"

const tt::hal::Configuration m5stack_cores3 = {
    .initBoot = initBoot,
    .initLvgl = initLvgl,
    .createDisplay = createDisplay,
    .sdcard = createSdCard(),
    .power = createPower,
    .i2c = {
        tt::hal::i2c::Configuration {
            .name = "Internal",
            .port = I2C_NUM_0,
            .initMode = tt::hal::i2c::InitMode::ByTactility,
            .canReinit = false,
            .hasMutableConfiguration = false,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_12,
                .scl_io_num = GPIO_NUM_11,
                .sda_pullup_en = true,
                .scl_pullup_en = true,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        },
        tt::hal::i2c::Configuration {
            .name = "External", // Grove
            .port = I2C_NUM_1,
            .initMode = tt::hal::i2c::InitMode::ByTactility,
            .canReinit = true,
            .hasMutableConfiguration = true,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_2,
                .scl_io_num = GPIO_NUM_1,
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
