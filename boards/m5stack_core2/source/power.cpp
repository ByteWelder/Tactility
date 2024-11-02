#include "power.h"
#include "M5Unified.hpp"

#ifdef __cplusplus
extern "C" {
#endif

static bool power_is_charging() {
    return M5.Power.isCharging() == m5::Power_Class::is_charging;
}

static void power_set_charging_enabled(bool enabled) {
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
    .set_charging_enabled = &power_set_charging_enabled,
    .get_charge_level = &power_get_charge_level,
    .get_current = &power_get_current
};

#ifdef __cplusplus
}
#endif
