#include "Hal/Hal_i.h"

#define TAG "hardware"

namespace tt::hal {

void init(const Configuration* configuration) {
    if (configuration->bootstrap != nullptr) {
        TT_LOG_I(TAG, "Bootstrapping");
        tt_check(configuration->bootstrap(), "bootstrap failed");
    }

    if (configuration->sdcard != nullptr) {
        TT_LOG_I(TAG, "Mounting sdcard");
        sdcard::mount(configuration->sdcard);
    }

    tt_check(configuration->init_graphics, "Graphics init not set");
    tt_check(configuration->init_graphics(), "Graphics init failed");
}

} // namespace
