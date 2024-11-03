#include "power.h"
#include "M5Unified.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * M5.Power by default doesn't have a check to see if charging is enabled.
 * However, it's always enabled by default after boot, so we cover that here:
 */
static bool is_charging_enabled = true;

static bool power_is_charging() {
    return M5.Power.isCharging() == m5::Power_Class::is_charging;
}

static bool power_is_charging_enabled() {
    return is_charging_enabled;
}

static void power_set_charging_enabled(bool enabled) {
    is_charging_enabled = enabled; // Local shadow copy because M5 API doesn't provide a function for it
    M5.Power.setBatteryCharge(enabled);
}

static uint8_t power_get_charge_level() {
    uint16_t scaled = (uint16_t)M5.Power.getBatteryLevel() * 255 / 100;
    return (uint8_t)scaled;
}

static int32_t power_get_current() {
    return M5.Power.getBatteryCurrent();
}

Power core2_power = {
    .is_charging = &power_is_charging,
    .is_charging_enabled = &power_is_charging_enabled,
    .set_charging_enabled = &power_set_charging_enabled,
    .get_charge_level = &power_get_charge_level,
    .get_current = &power_get_current
};

#ifdef __cplusplus
}
#endif
