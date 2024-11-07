#include "m5stack_cores3.h"
#include "m5stack_shared.h"

extern const SdCard m5stack_cores3_sdcard;

const HardwareConfig m5stack_cores3 = {
    .bootstrap = &m5stack_bootstrap,
    .display = {
        .set_backlight_duty = NULL
    },
    .init_graphics = &m5stack_lvgl_init,
    .sdcard = &m5stack_cores3_sdcard,
    .power = &m5stack_power
};
