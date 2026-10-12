// SPDX-License-Identifier: Apache-2.0
#include <mesh/mesh_airtime.h>

#include <algorithm>
#include <cmath>

uint32_t mesh_airtime_estimate_ms(const MeshRadioProfile& profile, size_t length) {
    if (profile.modulation == RADIO_MODULATION_LORA && profile.bandwidth > 0) {
        // Semtech AN1200.13, explicit header with CRC
        const double symbolMs = std::pow(2.0, profile.spreading_factor) * 1000.0 / profile.bandwidth;
        const int lowDataRateOptimize = (symbolMs > 16.0) ? 1 : 0;
        const int codingRate = std::clamp<int>(profile.coding_rate - 4, 1, 4);
        const double numerator = 8.0 * (double)length - 4.0 * profile.spreading_factor + 28.0 + 16.0;
        const double denominator = 4.0 * (profile.spreading_factor - 2 * lowDataRateOptimize);
        const double payloadSymbols = 8.0 + std::max(std::ceil(numerator / denominator) * (codingRate + 4), 0.0);
        const double preambleMs = (profile.preamble_length + 4.25) * symbolMs;
        return (uint32_t)std::ceil(preambleMs + payloadSymbols * symbolMs);
    }

    // FSK-family: preamble, sync word, length byte, payload and CRC. The data rate is not part of the
    // profile, so a conservative 4.8 kbit/s is assumed.
    constexpr double FALLBACK_BIT_RATE = 4800.0;
    const double bits = profile.preamble_length + (4 + 1 + length + 2) * 8.0;
    return (uint32_t)std::ceil(bits * 1000.0 / FALLBACK_BIT_RATE);
}

uint32_t mesh_airtime_duty_cycle_percent(RadioRegion region, int32_t frequency) {
    switch (region) {
        case RADIO_REGION_EU_868:
            return (frequency >= 869400000 && frequency <= 869650000) ? 10 : 1;
        case RADIO_REGION_EU_433:
            return 10;
        case RADIO_REGION_RU_868:
            return 1;
        default:
            return 100;
    }
}
