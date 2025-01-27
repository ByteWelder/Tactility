#include "UnPhonePower.h"
#include "UnPhoneTouch.h"
#include "Log.h"

#define TAG "unphone_power"

#define BATTERY_VOLTAGE_MIN 3.2f
#define BATTERY_VOLTAGE_MAX 4.2f

static uint8_t estimateChargeLevelFromVoltage(uint32_t milliVolt) {
    float volts = std::min((float)milliVolt / 1000.f, BATTERY_VOLTAGE_MAX);
    float voltage_percentage = (volts - BATTERY_VOLTAGE_MIN) / (BATTERY_VOLTAGE_MAX - BATTERY_VOLTAGE_MIN);
    float voltage_factor = std::min(1.0f, voltage_percentage);
    auto charge_level = (uint8_t) (voltage_factor * 100.f);
    TT_LOG_V(TAG, "mV = %lu, scaled = %.2f, factor = %.2f, result = %d", milliVolt, volts, voltage_factor, charge_level);
    return charge_level;
}

bool UnPhonePower::supportsMetric(MetricType type) const {
    switch (type) {
        case MetricType::BatteryVoltage:
        case MetricType::ChargeLevel:
            return true;
        default:
            return false;
    }
}

bool UnPhonePower::getMetric(Power::MetricType type, Power::MetricData& data) {
    switch (type) {
        case MetricType::BatteryVoltage:
            return readBatteryVoltageSampled(data.valueAsUint32);
        case MetricType::ChargeLevel: {
            uint32_t milli_volt;
            if (readBatteryVoltageSampled(milli_volt)) {
                data.valueAsUint8 = estimateChargeLevelFromVoltage(milli_volt);
                return true;
            } else {
                return false;
            }
        }
        default:
            return false;
    }
}

bool UnPhonePower::readBatteryVoltageOnce(uint32_t& output) const {
    auto* touch = UnPhoneTouch::getInstance();
    if (touch != nullptr) {
        float vbat;
        if (touch->getVBat(vbat)) {
            // Convert to mV
            output = (uint32_t)(vbat * 1000.f);
            return true;
        }
    }

    return false;
}


#define MAX_VOLTAGE_SAMPLES 15

bool UnPhonePower::readBatteryVoltageSampled(uint32_t& output) const {
    size_t samples_read = 0;
    uint32_t sample_accumulator = 0;
    uint32_t sample_read_buffer;

    for (size_t i = 0; i < MAX_VOLTAGE_SAMPLES; ++i) {
        if (readBatteryVoltageOnce(sample_read_buffer)) {
            sample_accumulator += sample_read_buffer;
            samples_read++;
        }
    }

    if (samples_read > 0) {
        output = sample_accumulator / samples_read;
        return true;
    } else {
        return false;
    }
}

static std::shared_ptr<Power> power;

std::shared_ptr<Power> unPhoneGetPower() {
    if (power == nullptr) {
        power = std::make_shared<UnPhonePower>();
    }
    return power;
}

