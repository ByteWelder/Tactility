#include "yellow_board.h"

bool twodotfour_lvgl_init();
bool twodotfour_bootstrap();

extern const SdCard twodotfour_sdcard;

const HardwareConfig yellow_board_24inch_cap = {
    .bootstrap = &twodotfour_bootstrap,
    .init_lvgl = &twodotfour_lvgl_init,
    .sdcard = &twodotfour_sdcard
};
