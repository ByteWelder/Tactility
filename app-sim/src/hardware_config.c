/**
 * Placeholder hardware config.
 * The real one happens during FreeRTOS startup. See freertos.c and lvgl_*.c
 */
#include <stdbool.h>
#include "hardware_config.h"


// TODO: See if we can move the init from FreeRTOS to app_main()?
static bool init_lvgl() { return true; }

HardwareConfig sim_hardware = {
    .bootstrap = NULL,
    .init_lvgl = &init_lvgl,
};

