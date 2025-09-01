#include "EstimatedPower.h"

bool EstimatedPower::supportsMetric(MetricType type) const {
    switch (type) {
        using enum MetricType;
        case BatteryVoltage:
        case ChargeLevel:
            return true;
        default:
            return false;
    }
}

bool EstimatedPower::getMetric(MetricType type, MetricData& data) {
    switch (type) {
        using enum MetricType;
        case BatteryVoltage:
            return chargeFromAdcVoltage->readBatteryVoltageSampled(data.valueAsUint32);
        case ChargeLevel:
            if (chargeFromAdcVoltage->readBatteryVoltageSampled(data.valueAsUint32)) {
                data.valueAsUint32 = chargeFromAdcVoltage->estimateChargeLevelFromVoltage(data.valueAsUint32);
                return true;
            } else {
                return false;
            }
        default:
            return false;
    }
}

