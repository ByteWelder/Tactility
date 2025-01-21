#pragma once

#ifdef ESP_PLATFORM
#include "sdkconfig.h"

// Supported hardware:
#if defined(CONFIG_TT_BOARD_LILYGO_TDECK)
#include "LilygoTdeck.h"
#define TT_BOARD_HARDWARE &lilygo_tdeck
#elif defined(CONFIG_TT_BOARD_CYD_2432S024C)
#include "CYD2432S024C.h"
#define TT_BOARD_HARDWARE &cyd_2432S024c_config
#elif defined(CONFIG_TT_BOARD_M5STACK_CORE2)
#include "M5stackCore2.h"
#define TT_BOARD_HARDWARE &m5stack_core2
#elif defined(CONFIG_TT_BOARD_M5STACK_CORES3)
#include "M5stackCoreS3.h"
#define TT_BOARD_HARDWARE &m5stack_cores3
#elif defined(CONFIG_TT_BOARD_UNPHONE)
#include "UnPhone.h"
#define TT_BOARD_HARDWARE &unPhone
#else
#define TT_BOARD_HARDWARE NULL
#error Replace TT_BOARD_HARDWARE in main.c with your own. Or copy one of the ./sdkconfig.board.* files into ./sdkconfig.
#endif

#else // else simulator

#include "Simulator.h"

extern tt::hal::Configuration hardware;
#define TT_BOARD_HARDWARE &hardware

#endif // ESP_PLATFORM