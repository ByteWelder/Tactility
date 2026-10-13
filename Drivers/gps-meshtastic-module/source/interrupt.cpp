// SPDX-License-Identifier: GPL-3.0
#include <gps_generic/private/interrupt.h>

#include <tactility/delay.h>

#include <algorithm>

constexpr uint32_t DELAY_SLICE_MILLIS = 50;

bool gps_delay_millis(const std::atomic<bool>& interrupted, uint32_t millis) {
    while (millis > 0) {
        if (interrupted) {
            return false;
        }
        const uint32_t slice = std::min(millis, DELAY_SLICE_MILLIS);
        delay_millis(slice);
        millis -= slice;
    }
    return !interrupted;
}
