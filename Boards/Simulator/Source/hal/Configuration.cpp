#include "hal/Configuration.h"
#include "LvglTask.h"
#include "src/lv_init.h"
#include "SdlDisplay.h"
#include "SdlKeyboard.h"

#define TAG "hardware"

extern const tt::hal::Power power;

static bool initBoot() {
    lv_init();
    lvgl_task_start();
    return true;
}

TT_UNUSED static void deinitPower() {
    lvgl_task_interrupt();
    while (lvgl_task_is_running()) {
        tt::delay_ms(10);
    }

#if LV_ENABLE_GC || !LV_MEM_CUSTOM
    lv_deinit();
#endif
}

extern const tt::hal::Configuration hardware = {
    .initBoot = initBoot,
    .createDisplay = createDisplay,
    .createKeyboard = createKeyboard,
    .sdcard = nullptr,
    .power = &power,
    .i2c = {
        tt::hal::i2c::Configuration {
            .name = "Internal",
            .port = I2C_NUM_0,
            .initMode = tt::hal::i2c::InitByTactility,
            .canReinit = false,
            .hasMutableConfiguration = false,
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
