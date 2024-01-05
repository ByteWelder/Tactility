#include "yellow_board.h"

DisplayDriver board_2432s024_create_display_driver();
TouchDriver board_2432s024_create_touch_driver();

const HardwareConfig yellow_board_24inch_cap = {
    .bootstrap = NULL,
    .display_driver = &board_2432s024_create_display_driver,
    .touch_driver = &board_2432s024_create_touch_driver
};
