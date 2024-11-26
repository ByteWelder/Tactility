#include "hal/Power.h"
#include "M5Unified.hpp"

/**
 * M5.Power by default doesn't have a check to see if charging is enabled.
 * However, it's always enabled by default after boot, so we cover that here:
 */
static bool charging_enabled = true;

static bool is_charging() {
    return M5.Power.isCharging() == m5::Power_Class::is_charging;
}

static bool is_charging_enabled() {
    return charging_enabled;
}

static void set_charging_enabled(bool enabled) {
    charging_enabled = enabled; // Local shadow copy because M5 API doesn't provide a function for it
    M5.Power.setBatteryCharge(enabled);
}

static uint8_t get_charge_level() {
    uint16_t scaled = (uint16_t)M5.Power.getBatteryLevel() * 255 / 100;
    return (uint8_t)scaled;
}

static int32_t get_current() {
    return M5.Power.getBatteryCurrent();
}

extern const tt::hal::Power m5stack_power = {
    .isCharging = &is_charging,
    .isChargingEnabled = &is_charging_enabled,
    .setChargingEnabled = &set_charging_enabled,
    .getChargeLevel = &get_charge_level,
    .getCurrent = &get_current
};
