// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <tactility/error.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Registers and starts the POSIX platform services.
 * The device scanner service scans /dev every second for serial and i2c-dev device files.
 * It registers a kernel device when a file appears and unregisters it when the file disappears.
 * Devices that are still in use when their file disappears stay registered until they can be stopped.
 * @retval ERROR_NONE on success
 */
error_t posix_services_start(void);

/**
 * Stops and unregisters the services that were started by posix_services_start().
 * @retval ERROR_NONE on success
 */
error_t posix_services_stop(void);

#ifdef __cplusplus
}
#endif
