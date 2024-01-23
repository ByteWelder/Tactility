#include "waveshare_s3_touch.h"

#include <stdbool.h>

bool waveshare_s3_touch_bootstrap();
bool waveshare_s3_touch_init_lvgl();

const HardwareConfig waveshare_s3_touch = {
    .bootstrap = &waveshare_s3_touch_bootstrap,
    .init_lvgl = &waveshare_s3_touch_init_lvgl
};
