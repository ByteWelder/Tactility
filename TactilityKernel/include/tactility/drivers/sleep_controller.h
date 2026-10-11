// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <tactility/device.h>
#include <tactility/error.h>

/**
 * @brief API for sleep controller drivers.
 */
struct SleepControllerApi {
    /**
     * @brief Puts the system into deep sleep, configured with the driver's wakeup sources.
     * @param[in] device the sleep controller device
     * @retval ERROR_NOT_SUPPORTED when the configured wakeup sources are not supported
     * @retval ERROR_NONE when the operation was successful (this call typically does not return)
     */
    error_t (*deep_sleep)(struct Device* device);
};

/**
 * @brief Puts the system into deep sleep using the specified sleep controller device.
 * Emits KERNEL_EVENT_DEEP_SLEEP_PREPARE synchronously before entering deep sleep.
 */
error_t sleep_controller_deep_sleep(struct Device* device);

extern const struct DeviceType SLEEP_CONTROLLER_TYPE;

#ifdef __cplusplus
}
#endif
