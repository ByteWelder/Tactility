// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdint.h>
#include <tactility/drivers/gpio.h>
#include <tactility/drivers/sleep_controller.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Esp32SleepConfig {
    /** Pins that wake the system from deep sleep when they become active */
    struct GpioPinSpec* wakeup_gpios;
    /** The item count of wakeup_gpios */
    uint8_t wakeup_gpios_count;
};

#ifdef __cplusplus
}
#endif
