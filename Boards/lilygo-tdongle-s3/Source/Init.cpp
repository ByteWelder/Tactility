#include "PwmBacklight.h"
#include "Tactility/service/gps/GpsService.h"

#include <Tactility/TactilityCore.h>

#define TAG "T-Dongle"

bool initBoot() {
    if (!driver::pwmbacklight::init(GPIO_NUM_38, 12000)) {
        TT_LOG_E(TAG, "Backlight init failed");
        return false;
    }

    return true;
}
