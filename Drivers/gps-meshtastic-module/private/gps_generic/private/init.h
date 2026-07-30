// SPDX-License-Identifier: GPL-3.0
#pragma once

#include <gps/gps.h>

struct Device;

/**
 * Sends the init sequence for a specific, already-probed GPS model over uart.
 */
bool gps_init(Device* uart, GpsModel model);
