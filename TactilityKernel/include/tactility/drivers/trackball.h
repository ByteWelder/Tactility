// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include <tactility/device.h>
#include <tactility/error.h>

/**
 * @brief API for trackball drivers.
 * Reports raw, unscaled movement: sensitivity and mode (encoder vs. pointer) are UI concerns
 * layered on top by the consumer (see lvgl/devices/trackball.h), not something this driver knows about.
 */
struct TrackballApi {
    /**
     * @brief Reads the accumulated movement since the last read, then resets it to zero.
     * @param[in] device the trackball device
     * @param[out] out_dx horizontal movement (right positive), in raw pulses
     * @param[out] out_dy vertical movement (down positive), in raw pulses
     * @retval ERROR_NONE when the operation was successful
     */
    error_t (*read_delta)(struct Device* device, int32_t* out_dx, int32_t* out_dy);

    /**
     * @brief Gets whether the click button is currently pressed.
     * @param[in] device the trackball device
     * @param[out] out_pressed true when pressed
     * @retval ERROR_NONE when the operation was successful
     */
    error_t (*get_button_pressed)(struct Device* device, bool* out_pressed);
};

/**
 * @brief Reads the accumulated movement using the specified trackball device.
 */
error_t trackball_read_delta(struct Device* device, int32_t* out_dx, int32_t* out_dy);

/**
 * @brief Gets whether the click button is currently pressed on the specified trackball device.
 */
error_t trackball_get_button_pressed(struct Device* device, bool* out_pressed);

extern const struct DeviceType TRACKBALL_TYPE;

#ifdef __cplusplus
}
#endif
