#include "yellow_board.h"
#include "display_i.h"

bool twodotfour_lvgl_init();
bool twodotfour_bootstrap();

extern const tt::hal::sdcard::SdCard twodotfour_sdcard;

const tt::hal::Configuration yellow_board_24inch_cap = {
    .bootstrap = &twodotfour_bootstrap,
    .init_graphics = &twodotfour_lvgl_init,
    .display = {
        .set_backlight_duty = &twodotfour_backlight_set
    },
    .sdcard = &twodotfour_sdcard,
    .power = nullptr
};
