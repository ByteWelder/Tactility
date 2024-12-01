#include "hal/Power.h"

static bool is_charging_enabled = false;

static bool isCharging() {
    return is_charging_enabled;
}

static bool isChargingEnabled() {
    return is_charging_enabled;
}

static void setChargingEnabled(bool enabled) {
    is_charging_enabled = enabled;
}

static uint8_t getChargeLevel() {
    return 204;
}

static int32_t getCurrent() {
    return is_charging_enabled ? 100 : -50;
}

extern const tt::hal::Power power = {
    .isCharging = isCharging,
    .isChargingEnabled = isChargingEnabled,
    .setChargingEnabled = setChargingEnabled,
    .getChargeLevel = getChargeLevel,
    .getCurrent = getCurrent
};
