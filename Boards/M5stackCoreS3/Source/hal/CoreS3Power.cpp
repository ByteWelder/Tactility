#include "CoreS3Power.h"

bool CoreS3Power::supportsMetric(MetricType type) const {
    switch (type) {
        using enum MetricType;
        case BatteryVoltage:
        case IsCharging:
        case ChargeLevel:
            return true;
        default:
            return false;
    }

    return false; // Safety guard for when new enum values are introduced
}

bool CoreS3Power::getMetric(Power::MetricType type, Power::MetricData& data) {
    switch (type) {
        using enum MetricType;
        case BatteryVoltage: {
            float milliVolt;
            if (axpDevice->getBatteryVoltage(milliVolt)) {
                data.valueAsUint32 = (uint32_t)milliVolt;
                return true;
            } else {
                return false;
            }
        }
        case ChargeLevel: {
            float vbatMillis;
            if (axpDevice->getBatteryVoltage(vbatMillis)) {
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
        case IsCharging: {
            Axp2101::ChargeStatus status;
            if (axpDevice->getChargeStatus(status)) {
                data.valueAsBool = (status == Axp2101::CHARGE_STATUS_CHARGING);
                return true;
            } else {
                return false;
            }
        }
        default:
            return false;
    }
}

bool CoreS3Power::isAllowedToCharge() const {
    bool enabled;
    if (axpDevice->isChargingEnabled(enabled)) {
        return enabled;
    } else {
        return false;
    }
}

void CoreS3Power::setAllowedToCharge(bool canCharge) {
    axpDevice->setChargingEnabled(canCharge);
}

static std::shared_ptr<Power> power;
extern std::shared_ptr<Axp2101> axp2101;

std::shared_ptr<Power> createPower() {
    if (power == nullptr) {
        power = std::make_shared<CoreS3Power>(axp2101);
    }

    return power;
}
