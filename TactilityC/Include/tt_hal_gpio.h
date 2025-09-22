#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int GpioPin;
#define GPIO_NO_PIN -1

/** @warning The order must match tt::hal::gpio::Mode */
enum class GpioMode {
    Disable = 0,
    Input,
    Output,
    OutputOpenDrain,
    InputOutput,
    InputOutputOpenDrain
};

/** Configure a single pin */
bool tt_hal_gpio_configure(GpioPin pin, GpioMode mode, bool pullUp, bool pullDown);

/** Configure a set of pins defined by their bit index */
bool tt_hal_gpio_configure_with_pin_bitmask(uint64_t pinBitMask, GpioMode mode, bool pullUp, bool pullDown);

bool tt_hal_gpio_set_mode(GpioPin pin, GpioMode mode);

bool tt_hal_gpio_get_level(GpioPin pin);

bool tt_hal_gpio_set_level(GpioPin pin, bool level);

int tt_hal_gpio_get_pin_count();

#ifdef __cplusplus
}
#endif
