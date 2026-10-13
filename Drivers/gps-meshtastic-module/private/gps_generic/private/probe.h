// SPDX-License-Identifier: GPL-3.0
#pragma once

#include <gps/gps.h>

#include <atomic>

struct Device;

/**
 * Attempts to auto-detect the GPS/GNSS chipset connected via uart.
 * @return GPS_MODEL_UNKNOWN when no supported chipset responded or when interrupted
 */
GpsModel gps_probe(Device* uart, const std::atomic<bool>& interrupted);
