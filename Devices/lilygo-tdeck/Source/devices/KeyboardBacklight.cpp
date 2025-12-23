#include "KeyboardBacklight.h"
#include <KeyboardBacklight.h>  // Driver
#include <Tactility/hal/i2c/I2c.h>

bool KeyboardBacklightDevice::start() {
    if (initialized) {
        return true;
    }
    
    // T-Deck uses I2C_NUM_0 for internal peripherals
    initialized = driver::keyboardbacklight::init(I2C_NUM_0);
    return initialized;
}

bool KeyboardBacklightDevice::stop() {
    if (initialized) {
        // Turn off backlight on shutdown
        driver::keyboardbacklight::setBrightness(0);
        initialized = false;
    }
    return true;
}

bool KeyboardBacklightDevice::setBrightness(uint8_t brightness) {
    if (!initialized) {
        return false;
    }
    return driver::keyboardbacklight::setBrightness(brightness);
}

uint8_t KeyboardBacklightDevice::getBrightness() const {
    if (!initialized) {
        return 0;
    }
    return driver::keyboardbacklight::getBrightness();
}
