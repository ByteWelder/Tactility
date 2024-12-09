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

class PowerNew {
    struct Metric {
        enum Type {
            IS_CHARGING, // bool
            CURRENT, // uint32_t, mAh
            BATTERY_VOLTAGE, // float [0.0, ...]
            CHARGE_LEVEL, // float [0.0, 1.0]
        };
        union {
            bool valueAsBool;
            uint32_t valueAsUint32;
            float valueAsFloat;
        };
    };

    virtual bool isMetricSupported(Metric::Type type) = 0;

    /**
     * @return false when metric is not supported or (temporarily) not available.
     */
    virtual bool getMetric(Metric& metric) = 0;

    virtual bool supportsChargeControl() { return true; }
    virtual bool isAllowedToCharge() { return false; }
    virtual void setChargingAllowed(bool canCharge) { /* NO-OP*/ }
};

} // namespace tt
