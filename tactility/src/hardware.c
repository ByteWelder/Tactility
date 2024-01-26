#include "check.h"
#include "hardware_i.h"

#define TAG "hardware"

void tt_hardware_init(const HardwareConfig* config) {
    if (config->bootstrap != NULL) {
        TT_LOG_I(TAG, "Bootstrapping");
        config->bootstrap();
    }

    tt_check(config->init_lvgl);
    config->init_lvgl();
}
