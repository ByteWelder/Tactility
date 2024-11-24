#include "yellow_board.h"
#include "display_i.h"

bool twodotfour_lvgl_init();
bool twodotfour_bootstrap();

extern const tt::hal::sdcard::SdCard twodotfour_sdcard;

const tt::hal::Configuration yellow_board_24inch_cap = {
    .initPower = &twodotfour_bootstrap,
    .initLvgl = &twodotfour_lvgl_init,
    .display = {
        .setBacklightDuty = &twodotfour_backlight_set
    },
    .sdcard = &twodotfour_sdcard,
    .power = nullptr,
    .i2c = {
        tt::hal::i2c::Configuration {
            .port = I2C_NUM_0,
            .initMode = tt::hal::i2c::InitDisabled,
            .canReinit = true,
            .hasMutableConfiguration = true,
            .timeout = 1000,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_NC,
                .scl_io_num = GPIO_NUM_NC,
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
                .sda_io_num = GPIO_NUM_NC,
                .scl_io_num = GPIO_NUM_NC,
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
