#include "CoreS3Power.h"
#include "TactilityCore.h"

#define TAG "core2_power"

bool CoreS3Power::supportsMetric(MetricType type) const {
    switch (type) {
        case MetricType::BatteryVoltage:
        case MetricType::IsCharging:
        case MetricType::ChargeLevel:
            return true;
        case MetricType::Current:
            return false;
    }

    return false; // Safety guard for when new enum values are introduced
}

bool CoreS3Power::getMetric(Power::MetricType type, Power::MetricData& data) {
    switch (type) {
        case MetricType::BatteryVoltage: {
            float milliVolt;
            if (axpDevice.getBatteryVoltage(milliVolt)) {
                data.valueAsUint32 = (uint32_t)milliVolt;
                return true;
            } else {
                return false;
            }
        }
        case MetricType::ChargeLevel: {
            float vbatMillis;
            if (axpDevice.getBatteryVoltage(vbatMillis)) {
                float vbat = vbatMillis / 1000.f;
                float max_voltage = 4.20f;
                float min_voltage = 2.69f; // From M5Unified
                if (vbat > 2.69f) {
                    float charge_factor = (vbat - min_voltage) / (max_voltage - min_voltage);
                    data.valueAsUint8 = (uint8_t)(charge_factor * 100.f);
                } else {
                    data.valueAsUint8 = 0;
                }
                return true;
            } else {
                return false;
            }
        }
        case MetricType::IsCharging: {
            Axp2101::ChargeStatus status;
            if (axpDevice.getChargeStatus(status)) {
                data.valueAsBool = (status == Axp2101::CHARGE_STATUS_CHARGING);
                return true;
            } else {
                return false;
            }
        }
        case MetricType::Current:
            return false;
    }

    return false; // Safety guard for when new enum values are introduced
}

bool CoreS3Power::isAllowedToCharge() const {
    bool enabled;
    if (axpDevice.isChargingEnabled(enabled)) {
        return enabled;
    } else {
        return false;
    }
}

void CoreS3Power::setAllowedToCharge(bool canCharge) {
    axpDevice.setChargingEnabled(canCharge);
}

static std::shared_ptr<Power> power;

std::shared_ptr<Power> createPower() {
    if (power == nullptr) {
        power = std::make_shared<CoreS3Power>();
    }
    return power;
}
