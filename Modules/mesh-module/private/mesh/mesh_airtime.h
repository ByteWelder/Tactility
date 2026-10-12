// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <mesh/backend.h>

#include <cstddef>
#include <cstdint>

/** Estimated time-on-air of a frame in milliseconds */
uint32_t mesh_airtime_estimate_ms(const MeshRadioProfile& profile, size_t length);

/** Allowed duty cycle in percent for a frequency in a region, 100 when unrestricted */
uint32_t mesh_airtime_duty_cycle_percent(RadioRegion region, int32_t frequency);
