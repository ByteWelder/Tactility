#include "SimulatorPower.h"

#define TAG "simulator_power"

bool SimulatorPower::supportsMetric(MetricType type) const {
    switch (type) {
        case MetricType::IsCharging:
        case MetricType::Current:
        case MetricType::BatteryVoltage:
        case MetricType::ChargeLevel:
            return true;
    }

    return false; // Safety guard for when new enum values are introduced
}

bool SimulatorPower::getMetric(Power::MetricType type, Power::MetricData& data) {
    switch (type) {
        case MetricType::IsCharging:
            data.valueAsBool = true;
            return true;
        case MetricType::Current:
            data.valueAsInt32 = 42;
            return true;
        case MetricType::BatteryVoltage:
            data.valueAsUint32 = 4032;
            return true;
        case MetricType::ChargeLevel:
            data.valueAsUint8 = 100;
            return true;
    }

    return false; // Safety guard for when new enum values are introduced
}

static std::shared_ptr<Power> power;

std::shared_ptr<Power> simulatorPower() {
    if (power == nullptr) {
        power = std::make_shared<SimulatorPower>();
    }
    return power;
}

