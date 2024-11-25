#pragma once

#include <cstdint>

namespace tt::hal {

typedef bool (*PowerIsCharging)();
typedef bool (*PowerIsChargingEnabled)();
typedef void (*PowerSetChargingEnabled)(bool enabled);
typedef uint8_t (*PowerGetBatteryCharge)(); // Power value [0, 255] which maps to 0-100% charge
typedef int32_t (*PowerGetCurrent)(); // Consumption or charge current in mAh

typedef struct {
    PowerIsCharging isCharging;
    PowerIsChargingEnabled isChargingEnabled;
    PowerSetChargingEnabled setChargingEnabled;
    PowerGetBatteryCharge getChargeLevel;
    PowerGetCurrent getCurrent;
} Power;

} // namespace tt
