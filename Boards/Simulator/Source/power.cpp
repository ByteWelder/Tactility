#include "hal/Power.h"

static bool is_charging_enabled = false;

static bool power_is_charging() {
    return is_charging_enabled;
}

static bool power_is_charging_enabled() {
    return is_charging_enabled;
}

static void power_set_charging_enabled(bool enabled) {
    is_charging_enabled = enabled;
}

static uint8_t power_get_charge_level() {
    return 204;
}

static int32_t power_get_current() {
    return is_charging_enabled ? 100 : -50;
}

extern const tt::hal::Power power = {
    .isCharging = &power_is_charging,
    .isChargingEnabled = &power_is_charging_enabled,
    .setChargingEnabled = &power_set_charging_enabled,
    .getChargeLevel = &power_get_charge_level,
    .getCurrent = &power_get_current
};
