#include "PwmBacklight.h"

#include <Tactility/Logger.h>
#include <Tactility/service/gps/GpsService.h>
#include <Tactility/TactilityCore.h>

static const auto LOGGER = tt::Logger("T-Dongle S3");

bool initBoot() {
    if (!driver::pwmbacklight::init(GPIO_NUM_38, 12000)) {
        LOGGER.error("Backlight init failed");
        return false;
    }

    return true;
}
