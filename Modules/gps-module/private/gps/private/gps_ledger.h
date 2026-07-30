// SPDX-License-Identifier: Apache-2.0
#pragma once

/**
 * @brief Reconciles the device tree against the persisted GPS configurations (see
 * gps/gps_settings.h): constructs+adds (but does not start) a GPS_TYPE device for every
 * configuration that doesn't have one yet, and stops+destructs+removes any ledger-owned device
 * whose configuration has disappeared.
 *
 * Only ever touches devices the ledger itself created (tagged DEVICE_FLAG_DYNAMIC) - devicetree-
 * declared GPS_TYPE devices (tagged DEVICE_FLAG_DTS) are never constructed, started, stopped, or
 * destructed by the ledger.
 */
void gps_ledger_sync();

/**
 * @brief Stops+destructs+removes every ledger-owned device. Devicetree-declared GPS_TYPE devices
 * are left untouched.
 */
void gps_ledger_clear();
