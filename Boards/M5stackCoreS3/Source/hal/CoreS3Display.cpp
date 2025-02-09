#include "CoreS3Display.h"

#include "CoreS3Constants.h"
#include "CoreS3Touch.h"

#include <Tactility/Log.h>
#include <Tactility/hal/i2c/I2c.h>

#include <Ili934xDisplay.h>

#define TAG "cores3"

void setBacklightDuty(uint8_t backlightDuty) {
    const uint8_t voltage = 20 + ((8 * backlightDuty) / 255); // [0b00000, 0b11100] - under 20 is too dark
    // TODO: Refactor to use Axp2102 class with https://github.com/m5stack/M5Unified/blob/b8cfec7fed046242da7f7b8024a4e92004a51ff7/src/utility/AXP2101_Class.cpp#L42
    if (!tt::hal::i2c::masterWriteRegister(I2C_NUM_0, AXP2101_ADDRESS, 0x99, &voltage, 1, 1000)) { // Sets DLD01
        TT_LOG_E(TAG, "Failed to set display backlight voltage");
    }
}

std::shared_ptr<tt::hal::display::Display> createDisplay() {
    auto touch = std::make_shared<CoreS3Touch>();

    auto configuration = std::make_unique<Ili934xDisplay::Configuration>(
        SPI3_HOST,
        GPIO_NUM_3,
        GPIO_NUM_35,
        320,
        240,
        touch
    );

    configuration->backlightDutyFunction = ::setBacklightDuty;

    return std::make_shared<Ili934xDisplay>(std::move(configuration));
}
