#include "lilygo_tdeck.h"
#include "display_i.h"

bool tdeck_bootstrap();
bool tdeck_init_lvgl();

extern const tt::SdCard tdeck_sdcard;

extern const tt::HardwareConfig lilygo_tdeck = {
    .bootstrap = &tdeck_bootstrap,
    .init_graphics = &tdeck_init_lvgl,
    .display = {
        .set_backlight_duty = &tdeck_backlight_set
    },
    .sdcard = &tdeck_sdcard,
    .power = nullptr
};
