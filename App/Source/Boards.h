#pragma once

#ifdef ESP_PLATFORM
#include "sdkconfig.h"

// Supported hardware:
#if defined(CONFIG_TT_BOARD_LILYGO_TDECK)
#include "lilygo_tdeck.h"
#define TT_BOARD_HARDWARE &lilygo_tdeck
#elif defined(CONFIG_TT_BOARD_YELLOW_BOARD_24_CAP)
#include "yellow_board.h"
#define TT_BOARD_HARDWARE &yellow_board_24inch_cap
#elif defined(CONFIG_TT_BOARD_M5STACK_CORE2)
#include "M5stackCore2.h"
#define TT_BOARD_HARDWARE &m5stack_core2
#elif defined(CONFIG_TT_BOARD_M5STACK_CORES3)
#include "M5stackCoreS3.h"
#define TT_BOARD_HARDWARE &m5stack_cores3
#elif defined(CONFIG_TT_BOARD_WAVESHARE_S3_TOUCH)
#include "waveshare_s3_touch.h"
#define TT_BOARD_HARDWARE &waveshare_s3_touch
#else
#define TT_BOARD_HARDWARE NULL
#error Replace TT_BOARD_HARDWARE in main.c with your own. Or copy one of the ./sdkconfig.board.* files into ./sdkconfig.
#endif

#else // else simulator

#include "Simulator.h"

#define TT_BOARD_HARDWARE &sim_hardware

extern "C" {
void app_main();
}

int main_stub(); // Main function logic from Simulator board project

// Actual main that passes on app_main (to be executed in a FreeRTOS task) and bootstraps FreeRTOS
int main() {
    setMainForSim(app_main);
    return main_stub();
}

#endif // ESP_PLATFORM