#pragma once

#include <cstdint>

namespace tt::hal::gpio {

typedef unsigned int Pin;
constexpr Pin NO_PIN = -1;

/** @warning The order must match GpioMode from tt_hal_gpio.h */
enum class Mode {
    Disable = 0,
    Input,
    Output,
    OutputOpenDrain,
    InputOutput,
    InputOutputOpenDrain
};

/** Configure a single pin */
bool configure(Pin pin, Mode mode, bool pullUp, bool pullDown);

/** Configure a set of pins defined by their bit index */
bool configureWithPinBitmask(uint64_t pinBitMask, Mode mode, bool pullUp, bool pullDown);

bool setMode(Pin pin, Mode mode);

bool getLevel(Pin pin);

bool setLevel(Pin pin, bool level);

int getPinCount();

}
