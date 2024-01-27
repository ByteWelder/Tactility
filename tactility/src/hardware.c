#include "hardware_i.h"

#include "check.h"
#include "lvgl.h"

#define TAG "hardware"

void tt_hardware_init(const HardwareConfig* config) {
    if (config->bootstrap != NULL) {
        TT_LOG_I(TAG, "Bootstrapping");
        tt_check(config->bootstrap(), "bootstrap failed");
    }

    tt_check(config->init_lvgl, "lvlg init not set");
    tt_check(config->init_lvgl(), "lvgl init failed");
}
