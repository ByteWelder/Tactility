#include "m5stack_core2.h"
#include "bootstrap.h"
#include "lvgl_i.h"

extern const SdCard core2_sdcard;

const HardwareConfig m5stack_core2 = {
    .bootstrap = &core2_bootstrap,
    .display = {
        .set_backlight_duty = NULL
    },
    .init_lvgl = &core2_lvgl_init,
    .sdcard = &core2_sdcard
};
