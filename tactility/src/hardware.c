#include "hardware_i.h"
#include "sdcard_i.h"

#define TAG "hardware"

typedef struct {
    bool initialized;
} Hardware;

void tt_hardware_init(const HardwareConfig* config) {
    if (config->bootstrap != NULL) {
        TT_LOG_I(TAG, "Bootstrapping");
        tt_check(config->bootstrap(), "bootstrap failed");
    }

    tt_sdcard_init();
    if (config->sdcard != NULL) {
        TT_LOG_I(TAG, "Mounting sdcard");
        tt_sdcard_mount(config->sdcard);
    }

    tt_check(config->init_lvgl, "lvlg init not set");
    tt_check(config->init_lvgl(), "lvgl init failed");
}
