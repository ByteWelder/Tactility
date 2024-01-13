#include "yellow_board.h"

const HardwareConfig yellow_board_24inch_cap = {
    .bootstrap = NULL,
    .display_driver = &board_2432s024_create_display_driver,
    .touch_driver = &board_2432s024_create_touch_driver
};
