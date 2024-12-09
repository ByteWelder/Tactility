#include "M5stackPower.h"

#include "M5Unified.h"

#define TAG "m5stack_power"

bool M5stackPower::supportsMetric(MetricType type) const {
    switch (type) {
        case IS_CHARGING:
        case CURRENT:
        case BATTERY_VOLTAGE:
        case CHARGE_LEVEL:
            return true;
    }

    return false; // Safety guard for when new enum values are introduced
}

bool M5stackPower::getMetric(Power::MetricType type, Power::MetricData& data) {
    switch (type) {
        case IS_CHARGING:
            data.valueAsBool = M5.Power.isCharging();
            return true;
        case CURRENT:
            data.valueAsInt32 = M5.Power.getBatteryCurrent();
            return true;
        case BATTERY_VOLTAGE:
            data.valueAsUint32 = M5.Power.getBatteryVoltage();
            return true;
        case CHARGE_LEVEL:
            data.valueAsUint8 = M5.Power.getBatteryLevel();
            return true;
    }

    return false; // Safety guard for when new enum values are introduced
}

void M5stackPower::setAllowedToCharge(bool canCharge) {
    M5.Power.setBatteryCharge(canCharge);
}

static std::shared_ptr<Power> power;

std::shared_ptr<Power> m5stack_get_power() {
    if (power == nullptr) {
        power = std::make_shared<M5stackPower>();
    }
    return power;
}

