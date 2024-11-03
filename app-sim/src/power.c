#include "power.h"

#ifdef __cplusplus
extern "C" {
#endif

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

const Power power = {
    .is_charging = &power_is_charging,
    .is_charging_enabled = &power_is_charging_enabled,
    .set_charging_enabled = &power_set_charging_enabled,
    .get_charge_level = &power_get_charge_level,
    .get_current = &power_get_current
};

#ifdef __cplusplus
}
#endif
