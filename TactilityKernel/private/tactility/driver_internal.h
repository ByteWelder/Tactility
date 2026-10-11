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

#ifdef __cplusplus
}
#endif
