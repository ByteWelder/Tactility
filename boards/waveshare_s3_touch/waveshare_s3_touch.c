#include "waveshare_s3_touch.h"

#include "lvgl_i.h"

bool ws3t_bootstrap();

const HardwareConfig waveshare_s3_touch = {
    .bootstrap = &ws3t_bootstrap,
    .init_lvgl = &ws3t_init_lvgl
};
