#pragma once

#include "sdkconfig.h"

// Supported hardware:
#if defined(CONFIG_TT_BOARD_LILYGO_TDECK)
#include "lilygo_tdeck.h"
#define TT_BOARD_HARDWARE &lilygo_tdeck
#elif defined(CONFIG_TT_BOARD_YELLOW_BOARD_24_CAP)
#include "yellow_board.h"
#define TT_BOARD_HARDWARE &yellow_board_24inch_cap
#else
#define TT_BOARD_HARDWARE NULL
#error Replace TT_BOARD_HARDWARE in main.c with your own. Or copy one of the ./sdkconfig.board.* files into ./sdkconfig.
#endif
