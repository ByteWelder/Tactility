#include "Hal/Hal_i.h"
#include "Hal/I2c/I2c.h"

#define TAG "hardware"

namespace tt::hal {

void init(const Configuration& configuration) {
    if (configuration.initPower != nullptr) {
        TT_LOG_I(TAG, "Init power");
        tt_check(configuration.initPower(), "Init power failed");
    }

    tt_check(i2c::init(configuration.i2c), "I2C init failed");
    if (configuration.initHardware != nullptr) {
        TT_LOG_I(TAG, "Init hardware");
        tt_check(configuration.initHardware(), "Hardware init failed");
    }

    if (configuration.sdcard != nullptr) {
        TT_LOG_I(TAG, "Mounting sdcard");
        if (!sdcard::mount(configuration.sdcard)) {
            TT_LOG_W(TAG, "SD card mount failed (init can continue)");
        }
    }

    tt_check(configuration.initLvgl, "Graphics init not set");
    tt_check(configuration.initLvgl(), "Graphics init failed");
}

} // namespace
