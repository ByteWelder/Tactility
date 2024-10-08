#include "waveshare_s3_touch.h"

#include "lvgl_i.h"

bool ws3t_bootstrap();

const HardwareConfig waveshare_s3_touch = {
    .bootstrap = &ws3t_bootstrap,
    .display = {
        .set_backlight_duty = NULL // TODO: This requires implementing the CH422G IO expander
    },
    .init_graphics = &ws3t_init_lvgl
};
