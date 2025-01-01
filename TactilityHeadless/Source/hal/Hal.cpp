#include "hal/Hal_i.h"
#include "hal/i2c/I2c.h"

#define TAG "hal"

namespace tt::hal {

void init(const Configuration& configuration) {
    tt_check(i2c::init(configuration.i2c), "I2C init failed");
    if (configuration.initHardware != nullptr) {
        TT_LOG_I(TAG, "Init hardware");
        tt_check(configuration.initHardware(), "Hardware init failed");
    }

    if (configuration.initBoot != nullptr) {
        TT_LOG_I(TAG, "Init power");
        tt_check(configuration.initBoot(), "Init power failed");
    }

    if (configuration.sdcard != nullptr) {
        TT_LOG_I(TAG, "Mounting sdcard");
        if (!configuration.sdcard->mount(TT_SDCARD_MOUNT_POINT )) {
            TT_LOG_W(TAG, "SD card mount failed (init can continue)");
        }
    }
}

} // namespace
