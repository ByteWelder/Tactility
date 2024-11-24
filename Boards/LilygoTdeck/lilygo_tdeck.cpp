#include "lilygo_tdeck.h"
#include "display.h"

bool tdeck_init_power();
bool tdeck_init_hardware();
bool tdeck_init_lvgl();

extern const tt::hal::sdcard::SdCard tdeck_sdcard;

extern const tt::hal::Configuration lilygo_tdeck = {
    .initPower = tdeck_init_power,
    .initHardware = tdeck_init_hardware,
    .initLvgl = &tdeck_init_lvgl,
    .display = {
        .setBacklightDuty = &tdeck_backlight_set
    },
    .sdcard = &tdeck_sdcard,
    .power = nullptr,
    .i2c = {
        tt::hal::i2c::Configuration {
            .port = I2C_NUM_0,
            .initMode = tt::hal::i2c::InitByTactility,
            .canReinit = false,
            .hasMutableConfiguration = false,
            .timeout = 1000,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_18,
                .scl_io_num = GPIO_NUM_8,
                .sda_pullup_en = GPIO_PULLUP_DISABLE,
                .scl_pullup_en = GPIO_PULLUP_DISABLE,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        },
        tt::hal::i2c::Configuration {
            .port = I2C_NUM_1,
            .initMode = tt::hal::i2c::InitDisabled,
            .canReinit = true,
            .hasMutableConfiguration = true,
            .timeout = 1000,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_43,
                .scl_io_num = GPIO_NUM_44,
                .sda_pullup_en = GPIO_PULLUP_DISABLE,
                .scl_pullup_en = GPIO_PULLUP_DISABLE,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        }
    }
};
