#include "SimulatorPower.h"

#define TAG "simulator_power"

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

static std::shared_ptr<PowerDevice> power;

std::shared_ptr<PowerDevice> simulatorPower() {
    if (power == nullptr) {
        power = std::make_shared<SimulatorPower>();
    }
    return power;
}

