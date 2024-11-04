#include "m5stack_cores3.h"
#include "bootstrap.h"
#include "lvgl_i.h"

extern const SdCard cores3_sdcard;
extern Power cores3_power; // Making it const fails the build

const HardwareConfig m5stack_cores3 = {
    .bootstrap = &cores3_bootstrap,
    .display = {
        .set_backlight_duty = NULL
    },
    .init_graphics = &cores3_lvgl_init,
    .sdcard = &cores3_sdcard,
    .power = &cores3_power
};
