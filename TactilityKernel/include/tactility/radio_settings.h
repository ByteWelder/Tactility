// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <tactility/drivers/radio.h>
#include <tactility/error.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @return the persisted regulatory region, RADIO_REGION_UNSET when none was saved */
enum RadioRegion radio_settings_load_region(void);

/**
 * Persist the regulatory region and apply it to all radio devices.
 * @retval ERROR_NOT_FOUND when the user data storage isn't available
 * @retval ERROR_RESOURCE when writing failed
 */
error_t radio_settings_save_region(enum RadioRegion region);

/** Apply the persisted region to all radio devices */
void radio_settings_apply_region(void);

#ifdef __cplusplus
}
#endif
