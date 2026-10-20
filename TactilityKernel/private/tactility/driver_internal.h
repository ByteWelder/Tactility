// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <tactility/driver.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Set the module that owns a driver.
 *
 * @param driver The constructed driver.
 * @param owner The owning module, or NULL.
 */
void driver_set_owner(struct Driver* driver, const struct Module* owner);

/**
 * @brief Stop new devices from binding to a driver, unless a device is bound to it already.
 *
 * Checking and refusing are one atomic step, so a bind can't slip in between. A successful call
 * guarantees driver_remove_destruct() can't fail because of a bound device.
 *
 * @param driver The constructed driver.
 * @retval ERROR_RESOURCE_BUSY a device is bound to the driver. Nothing was changed.
 * @retval ERROR_NONE binds are refused from now on, until driver_cancel_destroy().
 */
error_t driver_begin_destroy(struct Driver* driver);

/**
 * @brief Allow devices to bind to a driver again after a successful driver_begin_destroy().
 *
 * @param driver The constructed driver.
 */
void driver_cancel_destroy(struct Driver* driver);

#ifdef __cplusplus
}
#endif
