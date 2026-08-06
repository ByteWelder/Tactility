// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include <gps/gps.h>
#include <tactility/error.h>

/**
 * @brief A persisted GPS receiver configuration.
 */
struct GpsConfiguration {
    /** UART controller device name, e.g. "uart0" - resolved via device_get_by_name(). */
    char uart_name[32];
    uint32_t baud_rate;
    /** GPS_MODEL_UNKNOWN triggers an autoprobe. */
    enum GpsModel model;
};

/**
 * @brief Persists a new GPS configuration and triggers the ledger to materialize a (not started)
 * GPS_TYPE device for it in the device tree. Use device_start()/device_stop() on the resulting
 * device to control whether it's actually running.
 * @retval ERROR_RESOURCE if the configuration file could not be opened/written
 */
error_t gps_settings_add_configuration(const struct GpsConfiguration* configuration);

/**
 * @brief Removes the persisted GPS configuration at `index` (as seen via
 * gps_settings_for_each_configuration()), and triggers the ledger to stop, destruct and remove
 * its corresponding GPS_TYPE device from the device tree.
 * @retval ERROR_NOT_FOUND if index is out of range
 * @retval ERROR_RESOURCE if the configuration file could not be read/written
 */
error_t gps_settings_remove_configuration_at(size_t index);

/**
 * @brief Iterates over all persisted GPS configurations.
 * @param[in] context passed through to on_configuration, can be NULL
 * @param[in] on_configuration called once per configuration, in file order, with its index
 */
void gps_settings_for_each_configuration(void* context, void (*on_configuration)(const struct GpsConfiguration* configuration, size_t index, void* context));

#ifdef __cplusplus
}
#endif
