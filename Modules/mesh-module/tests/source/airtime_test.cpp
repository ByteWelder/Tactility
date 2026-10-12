#include "doctest.h"

#include <mesh/mesh_airtime.h>

TEST_CASE("LoRa airtime matches the Semtech calculator") {
    // SF11, 250 kHz, CR 4/5, preamble 16, 32 byte payload: 477.2 ms
    const MeshRadioProfile profile = {
        .modulation = RADIO_MODULATION_LORA,
        .frequency = 869525000,
        .bandwidth = 250000,
        .spreading_factor = 11,
        .coding_rate = 5,
        .sync_word = 0x2B,
        .preamble_length = 16,
        .power = 14,
    };
    const uint32_t airtime = mesh_airtime_estimate_ms(profile, 32);
    CHECK_EQ(airtime, 478);
}

TEST_CASE("Duty cycle depends on region and sub-band") {
    CHECK_EQ(mesh_airtime_duty_cycle_percent(RADIO_REGION_EU_868, 869525000), 10);
    CHECK_EQ(mesh_airtime_duty_cycle_percent(RADIO_REGION_EU_868, 868100000), 1);
    CHECK_EQ(mesh_airtime_duty_cycle_percent(RADIO_REGION_US_915, 906875000), 100);
}
