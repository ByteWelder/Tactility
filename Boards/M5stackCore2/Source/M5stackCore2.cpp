#include "M5stackCore2.h"
#include "m5stack_shared.h"

extern const tt::hal::sdcard::SdCard m5stack_core2_sdcard;

extern const tt::hal::Configuration m5stack_core2 = {
    .bootstrap = &m5stack_bootstrap,
    .init_graphics = &m5stack_lvgl_init,
    .display = {
        .set_backlight_duty = nullptr
    },
    .sdcard = &m5stack_core2_sdcard,
    .power = &m5stack_power
};
