// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <gps/gps.h>
#include <tactility/device.h>

/**
 * @brief Devicetree configuration for a generic UART-connected GPS/GNSS receiver.
 */
struct GpsConfig {
    uint32_t baud_rate;
    /** GPS_MODEL_UNKNOWN triggers an autoprobe on start(); the detected model is then available via get_model(). */
    enum GpsModel model;
};

#ifdef __cplusplus
}
#endif
