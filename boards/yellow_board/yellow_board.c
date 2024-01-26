#include "yellow_board.h"

bool yellow_board_init_lvgl();

const HardwareConfig yellow_board_24inch_cap = {
    .bootstrap = NULL,
    .init_lvgl = &yellow_board_init_lvgl
};
