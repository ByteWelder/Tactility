#include "Core2Power.h"

#include "utility/AXP192_Class.hpp"
#include "TactilityCore.h"

#define TAG "core2_power"

extern m5::AXP192_Class axpDevice;

bool Core2Power::supportsMetric(MetricType type) const {
    switch (type) {
        case BATTERY_VOLTAGE:
        case CHARGE_LEVEL:
        case IS_CHARGING:
            return true;
        case CURRENT:
            return false;
    }

    return false; // Safety guard for when new enum values are introduced
}

bool Core2Power::getMetric(Power::MetricType type, Power::MetricData& data) {
    switch (type) {
        case BATTERY_VOLTAGE:
            data.valueAsUint32 = (uint32_t)TT_MAX((axpDevice.getBatteryVoltage() * 1000.f), 0.0f);
            return true;
        case CHARGE_LEVEL: {
            auto level = axpDevice.getBatteryLevel();
            if (level > 0) {
                data.valueAsUint8 = (uint8_t)level;
                return true;
            } else {
                return false;
            }
        }
        case IS_CHARGING:
            data.valueAsBool = axpDevice.isCharging();
            return true;
        case CURRENT:
            return false;
    }

    return false; // Safety guard for when new enum values are introduced
}

bool Core2Power::isAllowedToCharge() const {
    return allowedToCharge;
}

void Core2Power::setAllowedToCharge(bool canCharge) {
    allowedToCharge = canCharge;
    axpDevice.setBatteryCharge(canCharge);
}

static std::shared_ptr<Power> power;

std::shared_ptr<Power> createPower() {
    if (power == nullptr) {
        power = std::make_shared<Core2Power>();
    }
    return power;
}
