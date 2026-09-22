// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <tactility/device.h>

/**
 * @brief Devicetree configuration for an SH8601 panel's backlight.
 *
 * The SH8601 has no separate backlight control line; brightness is set with the panel's own
 * 0x51 command, sent through the parent display device's panel IO.
 */
struct Sh8601BacklightConfig {
    /** Default brightness level (0-255), applied by set_brightness_default() */
    uint8_t brightness_default;
};

#ifdef __cplusplus
}
#endif
