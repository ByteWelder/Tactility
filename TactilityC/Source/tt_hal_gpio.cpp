#include "tt_hal_gpio.h"
#include <Tactility/hal/gpio/Gpio.h>

extern "C" {

using namespace tt::hal;

bool tt_hal_gpio_configure(GpioPin pin, GpioMode mode, bool pullUp, bool pullDown) {
    return gpio::configure(pin, static_cast<gpio::Mode>(mode), pullUp, pullDown);
}

bool tt_hal_gpio_configure_with_pin_bitmask(uint64_t pinBitMask, GpioMode mode, bool pullUp, bool pullDown) {
    return gpio::configureWithPinBitmask(pinBitMask, static_cast<gpio::Mode>(mode), pullUp, pullDown);
}

bool tt_hal_gpio_set_mode(GpioPin pin, GpioMode mode) {
    return gpio::setMode(pin, static_cast<gpio::Mode>(mode));
}

bool tt_hal_gpio_get_level(GpioPin pin) {
    return gpio::getLevel(pin);
}

bool tt_hal_gpio_set_level(GpioPin pin, bool level) {
    return gpio::setLevel(pin, level);
}

int tt_hal_gpio_get_pin_count() {
    return gpio::getPinCount();
}

}
