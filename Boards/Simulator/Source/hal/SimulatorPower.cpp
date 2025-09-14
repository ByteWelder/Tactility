#include "SimulatorPower.h"

constexpr auto* TAG = "SimulatorPower";

bool SimulatorPower::supportsMetric(MetricType type) const {
    switch (type) {
        using enum MetricType;
        case IsCharging:
        case Current:
        case BatteryVoltage:
        case ChargeLevel:
            return true;
    }

    return false; // Safety guard for when new enum values are introduced
}

bool SimulatorPower::getMetric(MetricType type, MetricData& data) {
    switch (type) {
        using enum MetricType;
        case IsCharging:
            data.valueAsBool = true;
            return true;
        case Current:
            data.valueAsInt32 = 42;
            return true;
        case BatteryVoltage:
            data.valueAsUint32 = 4032;
            return true;
        case ChargeLevel:
            data.valueAsUint8 = 100;
            return true;
    }

    return false; // Safety guard for when new enum values are introduced
}
