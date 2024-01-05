#pragma once

// Supported hardware:
#if defined(CONFIG_NB_BOARD_LILYGO_TDECK)
#include "lilygo_tdeck.h"
#define NB_BOARD_HARDWARE &lilygo_tdeck
#elif defined(CONFIG_NB_BOARD_YELLOW_BOARD_24_CAP)
#include "yellow_board.h"
#define NB_BOARD_HARDWARE &yellow_board_24inch_cap
#elif defined(CONFIG_NB_BOARD_CUSTOM)
#define NB_BOARD_HARDWARE furi_crash( \
    "Replace NB_BOARD_HARDWARE in main.c with your own, or use \"idf.py menuconfig\" to select a supported board." \
)
#endif
