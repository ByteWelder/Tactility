// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <sd_protocol_types.h>
#include <tactility/error.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Device;

/**
 * Try to get the sdmmc_card_t* for the given device.
 * @param device any device.
 * @return the sdmmc_card_t* if it is available for this device, otherwise return null
 */
sdmmc_card_t* esp32_sdcard_get_card(struct Device* device);

/**
 * @brief Wraps the card's do_transaction function pointer so every SD command takes the parent
 * SPI controller's bus lock.
 * @param[in] device the SD card device
 * @param[in] card the card returned by the mount call, passed directly rather than re-resolved
 *     via esp32_sdcard_get_card(): this is called from within the driver's own start_device
 *     callback, before device_is_ready() is true, so the readiness-gated getters can't be used
 *     here.
 * @retval ERROR_NONE on success, or when the device's parent is not a SPI controller (no-op,
 *     e.g. SDMMC cards, which have no shared bus to arbitrate)
 */
error_t esp32_sdcard_install_bus_lock(struct Device* device, sdmmc_card_t* card);

/**
 * @brief Removes the bus lock installed by esp32_sdcard_install_bus_lock(), restoring the card's
 * original do_transaction. No-op if no lock was installed for this device. Must be called before
 * the card handle is freed.
 * @param[in] device the SD card device
 */
void esp32_sdcard_remove_bus_lock(struct Device* device);

#ifdef __cplusplus
}
#endif
