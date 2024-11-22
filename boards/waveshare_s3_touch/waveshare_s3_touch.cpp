#include "waveshare_s3_touch.h"

#include "lvgl_i.h"

bool ws3t_bootstrap();

extern const tt::hal::Configuration waveshare_s3_touch = {
    .bootstrap = &ws3t_bootstrap,
    .init_graphics = &ws3t_init_lvgl,
    .display = {
        .set_backlight_duty = nullptr // TODO: This requires implementing the CH422G IO expander
    },
    .sdcard = nullptr,
    .power = nullptr
};
