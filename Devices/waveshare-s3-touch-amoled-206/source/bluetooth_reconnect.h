// SPDX-License-Identifier: Apache-2.0
#pragma once

/**
 * Board-local Bluetooth auto-reconnect for the Waveshare S3 Touch AMOLED 2.06.
 *
 * Starts a task that scans and reconnects saved (paired) BLE peers. It runs its own kernel BT
 * event subscription on its own thread instead of relying on Tactility's "bt-events" thread,
 * which can block while the radio is on.
 */
void waveshare_bluetooth_reconnect_start();

/** Stops the task, if running. */
void waveshare_bluetooth_reconnect_stop();
