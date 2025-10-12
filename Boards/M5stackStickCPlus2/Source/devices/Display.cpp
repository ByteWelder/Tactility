#include "Display.h"

#include <Axp2101.h>
#include <Ft5x06Touch.h>
#include <Ili934xDisplay.h>
#include <Tactility/Log.h>
#include <Tactility/hal/i2c/I2c.h>

constexpr auto* TAG = "CoreS3Display";

static void setBacklightDuty(uint8_t backlightDuty) {
    const uint8_t voltage = 20 + ((8 * backlightDuty) / 255); // [0b00000, 0b11100] - under 20 is too dark
    // TODO: Refactor to use Axp2102 driver subproject. Reference: https://github.com/m5stack/M5Unified/blob/b8cfec7fed046242da7f7b8024a4e92004a51ff7/src/utility/AXP2101_Class.cpp#L42
    if (!tt::hal::i2c::masterWriteRegister(I2C_NUM_0, AXP2101_ADDRESS, 0x99, &voltage, 1, 1000)) { // Sets DLD01
        TT_LOG_E(TAG, "Failed to set display backlight voltage");
    }
}

static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    // Note for future changes: Reset pin is 48 and interrupt pin is 47
    auto configuration = std::make_unique<Ft5x06Touch::Configuration>(
        I2C_NUM_0,
        320,
        240
    );

    auto touch = std::make_shared<Ft5x06Touch>(std::move(configuration));
    return std::reinterpret_pointer_cast<tt::hal::touch::TouchDevice>(touch);
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createTouch();

    auto configuration = std::make_unique<Ili934xDisplay::Configuration>(
        SPI3_HOST,
        GPIO_NUM_3,
        GPIO_NUM_35,
        320,
        240,
        touch,
        false,
        false,
        false,
        true
    );

    configuration->backlightDutyFunction = ::setBacklightDuty;

    auto display = std::make_shared<Ili934xDisplay>(std::move(configuration));
    return std::reinterpret_pointer_cast<tt::hal::display::DisplayDevice>(display);
}
