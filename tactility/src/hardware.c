#include "hardware_i.h"

#include "check.h"
#include "lvgl.h"

#define TAG "hardware"

#define SDCARD_MOUNT_POINT "/sdcard"

void tt_hardware_init(const HardwareConfig* config) {
    if (config->bootstrap != NULL) {
        TT_LOG_I(TAG, "Bootstrapping");
        tt_check(config->bootstrap(), "bootstrap failed");
    }

    if (config->sdcard != NULL) {
        TT_LOG_I(TAG, "Mounting sdcard");
        void* sdcard_context = config->sdcard->mount(SDCARD_MOUNT_POINT);
        config->sdcard->unmount(sdcard_context);
    }

    tt_check(config->init_lvgl, "lvlg init not set");
    tt_check(config->init_lvgl(), "lvgl init failed");
}
