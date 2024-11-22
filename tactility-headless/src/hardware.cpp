#include "hardware_i.h"

#define TAG "hardware"

namespace tt {

void hardware_init(const HardwareConfig* config) {
    if (config->bootstrap != nullptr) {
        TT_LOG_I(TAG, "Bootstrapping");
        tt_check(config->bootstrap(), "bootstrap failed");
    }

    if (config->sdcard != nullptr) {
        TT_LOG_I(TAG, "Mounting sdcard");
        tt_sdcard_mount(config->sdcard);
    }

    tt_check(config->init_graphics, "Graphics init not set");
    tt_check(config->init_graphics(), "Graphics init failed");
}

} // namespace
