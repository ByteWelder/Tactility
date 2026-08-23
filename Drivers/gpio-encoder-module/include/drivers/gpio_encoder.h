// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <tactility/drivers/gpio.h>

struct GpioEncoderConfig {
    // First pin of encoder wheel
    struct GpioPinSpec pin_a;
    // Second pin of encoder wheel
    struct GpioPinSpec pin_b;
    // "Button" pin of encoder wheel. Optional: GPIO_PIN_SPEC_NONE when the wheel has no click/enter button.
    struct GpioPinSpec pin_enter;
    // Quadrature pulses per mechanical detent (x4 decode gives 4 pulses/detent for a standard EC11-style encoder).
    uint8_t pulses_per_detent;
    // Capacity of the queue buffering key events between read_key() polls.
    uint8_t pending_capacity;
};

#ifdef __cplusplus
}
#endif
