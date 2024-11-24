#include "Hal/Configuration.h"
#include "lvgl_task.h"
#include "src/lv_init.h"

#define TAG "hardware"

extern const tt::hal::Power power;

static bool lvgl_init() {
    lv_init();
    lvgl_task_start();
    return true;
}

TT_UNUSED static void lvgl_deinit() {
    lvgl_task_interrupt();
    while (lvgl_task_is_running()) {
        tt::delay_ms(10);
    }

#if LV_ENABLE_GC || !LV_MEM_CUSTOM
    lv_deinit();
#endif
}

extern const tt::hal::Configuration sim_hardware = {
    .initLvgl = &lvgl_init,
    .display = {
        .setBacklightDuty = nullptr,
    },
    .sdcard = nullptr,
    .power = &power,
    .i2c = {
        tt::hal::i2c::Configuration {
            .name = "Internal",
            .port = I2C_NUM_0,
            .initMode = tt::hal::i2c::InitByTactility,
            .canReinit = false,
            .hasMutableConfiguration = false,
            .timeout = 1000,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = 1,
                .scl_io_num = 2,
                .sda_pullup_en = true,
                .scl_pullup_en = true,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        },
        tt::hal::i2c::Configuration {
            .name = "External",
            .port = I2C_NUM_1,
            .initMode = tt::hal::i2c::InitByTactility,
            .canReinit = true,
            .hasMutableConfiguration = true,
            .timeout = 1000,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = 1,
                .scl_io_num = 2,
                .sda_pullup_en = false,
                .scl_pullup_en = false,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        }
    }
};
