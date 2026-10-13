// SPDX-License-Identifier: GPL-3.0
#pragma once

#include <gps/gps.h>

#include <atomic>

struct Device;

/**
 * Sends the init sequence for a specific, already-probed GPS model over uart.
 * @return false on failure or when interrupted
 */
bool gps_init(Device* uart, GpsModel model, const std::atomic<bool>& interrupted);
