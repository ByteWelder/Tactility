#include "waveshare_s3_touch.h"

#include "lvgl_i.h"

bool ws3t_bootstrap();

extern const tt::hal::Configuration waveshare_s3_touch = {
    .initBoot = &ws3t_bootstrap,
    .initLvgl = &ws3t_init_lvgl,
    .display = { .setBacklightDuty = nullptr },
    .sdcard = nullptr,
    .power = nullptr,
    .i2c = {
        tt::hal::i2c::Configuration {
            .name = "First",
            .port = I2C_NUM_0,
            .initMode = tt::hal::i2c::InitDisabled,
            .canReinit = true,
            .hasMutableConfiguration = true,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_NC,
                .scl_io_num = GPIO_NUM_NC,
                .sda_pullup_en = true,
                .scl_pullup_en = true,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        },
        tt::hal::i2c::Configuration {
            .name = "Second",
            .port = I2C_NUM_1,
            .initMode = tt::hal::i2c::InitDisabled,
            .canReinit = true,
            .hasMutableConfiguration = true,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_NC,
                .scl_io_num = GPIO_NUM_NC,
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
