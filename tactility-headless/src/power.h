#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef bool (*PowerIsCharging)();
typedef bool (*PowerIsChargingEnabled)();
typedef void (*PowerSetChargingEnabled)(bool enabled);
typedef uint8_t (*PowerGetBatteryCharge)(); // Power value [0, 255] which maps to 0-100% charge
typedef int32_t (*PowerGetCurrent)(); // Consumption or charge current in mAh

typedef struct {
    PowerIsCharging is_charging;
    PowerIsChargingEnabled is_charging_enabled;
    PowerSetChargingEnabled set_charging_enabled;
    PowerGetBatteryCharge get_charge_level;
    PowerGetCurrent get_current;
} Power;

#ifdef __cplusplus
}
#endif