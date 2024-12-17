#include "SimulatorPower.h"

#define TAG "simulator_power"

bool SimulatorPower::supportsMetric(MetricType type) const {
    switch (type) {
        case IS_CHARGING:
        case CURRENT:
        case BATTERY_VOLTAGE:
        case CHARGE_LEVEL:
            return true;
    }

    return false; // Safety guard for when new enum values are introduced
}

bool SimulatorPower::getMetric(Power::MetricType type, Power::MetricData& data) {
    switch (type) {
        case IS_CHARGING:
            data.valueAsBool = true;
            return true;
        case CURRENT:
            data.valueAsInt32 = 42;
            return true;
        case BATTERY_VOLTAGE:
            data.valueAsUint32 = 4032;
            return true;
        case CHARGE_LEVEL:
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

