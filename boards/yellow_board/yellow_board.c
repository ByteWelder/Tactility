#include "yellow_board.h"
#include "display_i.h"

bool twodotfour_lvgl_init();
bool twodotfour_bootstrap();

extern const SdCard twodotfour_sdcard;

const HardwareConfig yellow_board_24inch_cap = {
    .bootstrap = &twodotfour_bootstrap,
    .display = {
        .set_backlight = &twodotfour_backlight_set
    },
    .init_lvgl = &twodotfour_lvgl_init,
    .sdcard = &twodotfour_sdcard
};
