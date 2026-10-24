#pragma once

#include <tactility/device.h>
#include <tactility/drivers/backlight.h>
#include <tactility/drivers/display.h>

#include <stdint.h>

namespace tt::backlight {

/**
 * The backlight of the device's first display, or nullptr when it has none.
 * Boards not yet migrated to the kernel display driver register a placeholder display device with
 * a NULL api, which has no backlight to hand out.
 * @warning The caller owns the returned device and must release it with device_put().
 */
inline Device* findDisplayBacklight() {
    Device* display = nullptr;
    if (device_get_first_by_type(&DISPLAY_TYPE, &display) != ERROR_NONE) {
        return nullptr;
    }
    if (device_get_driver(display)->api == nullptr) {
        device_put(display);
        return nullptr;
    }
    Device* found = nullptr;
    display_get_backlight(display, &found);
    device_put(display);
    return found;
}

/** @return true when the backlight can be dimmed, rather than only switched on and off */
inline bool isDimmable(Device* backlight) {
    const int32_t min = backlight_get_min_brightness(backlight);
    const int32_t max = backlight_get_max_brightness(backlight);
    // A 0-1 range is an on/off switch: every slider position would map to "on"
    return max > min && !(min == 0 && max == 1);
}

/** @return the backlight's device units as a percentage of its range */
inline int32_t toPercent(Device* backlight, int32_t brightness) {
    const int32_t min = backlight_get_min_brightness(backlight);
    const int32_t range = backlight_get_max_brightness(backlight) - min;
    return range > 0 ? ((brightness - min) * 100 + range / 2) / range : 100;
}

/** @return a percentage as the backlight's device units. 0% is the dimmest setting that is still on. */
inline int32_t toBrightness(Device* backlight, int32_t percent) {
    const int32_t min = backlight_get_min_brightness(backlight);
    const int32_t range = backlight_get_max_brightness(backlight) - min;
    const int32_t brightness = min + (percent * range + 50) / 100;
    // A brightness of 0 turns the backlight off
    return brightness < 1 ? 1 : brightness;
}

}
