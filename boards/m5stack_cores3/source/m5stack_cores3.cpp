#include "m5stack_cores3.h"
#include "m5stack_shared.h"

extern const tt::SdCard m5stack_cores3_sdcard;

const tt::HardwareConfig m5stack_cores3 = {
    .bootstrap = &m5stack_bootstrap,
    .init_graphics = &m5stack_lvgl_init,
    .display = {
        .set_backlight_duty = nullptr
    },
    .sdcard = &m5stack_cores3_sdcard,
    .power = &m5stack_power
};
