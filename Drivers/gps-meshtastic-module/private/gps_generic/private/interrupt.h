// SPDX-License-Identifier: GPL-3.0
#pragma once

#include <atomic>
#include <cstdint>

/**
 * Delays unless the GPS thread is interrupted.
 * @return false when interrupted
 */
bool gps_delay_millis(const std::atomic<bool>& interrupted, uint32_t millis);
