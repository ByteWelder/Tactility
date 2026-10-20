// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifdef ESP_PLATFORM
#include <sdkconfig.h>
#endif

#if defined(CONFIG_BT_NIMBLE_ENABLED)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Installs the filesystem-backed NimBLE security store (bond + CCCD records).
 *
 * Replaces IDF's NVS-backed store/config implementation: the in-RAM database and the
 * key-matching/iteration semantics are the same, but the records are persisted to a
 * single file on the data partition (/data, FAT) instead of NVS.
 *
 * Must be called instead of ble_store_config_init().
 */
void ble_store_fs_init(void);

#ifdef __cplusplus
}
#endif

#endif // CONFIG_BT_NIMBLE_ENABLED
