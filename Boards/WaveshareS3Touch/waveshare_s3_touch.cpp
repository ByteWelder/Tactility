#include "waveshare_s3_touch.h"

#include "lvgl_i.h"

bool ws3t_bootstrap();

extern const tt::hal::Configuration waveshare_s3_touch = {
    .initPower = &ws3t_bootstrap,
    .initLvgl = &ws3t_init_lvgl,
    .display = { .setBacklightDuty = nullptr },
    .sdcard = nullptr,
    .power = nullptr,
    .i2c = {}
};
