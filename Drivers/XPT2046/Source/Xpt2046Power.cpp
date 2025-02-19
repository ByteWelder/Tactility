#include "Xpt2046Power.h"
#include "Xpt2046Touch.h"

#include <Tactility/Log.h>

#define TAG "xpt2046_power"

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

bool Xpt2046Power::supportsMetric(MetricType type) const {
    switch (type) {
        using enum MetricType;
        case BatteryVoltage:
        case ChargeLevel:
            return true;
        default:
            return false;
    }
}

bool Xpt2046Power::getMetric(MetricType type, MetricData& data) {
    switch (type) {
        using enum MetricType;
        case BatteryVoltage:
            return readBatteryVoltageSampled(data.valueAsUint32);
        case ChargeLevel: {
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

bool Xpt2046Power::readBatteryVoltageOnce(uint32_t& output) const {
    // Make a safe copy
    auto touch = Xpt2046Touch::getInstance();
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

bool Xpt2046Power::readBatteryVoltageSampled(uint32_t& output) const {
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

static std::shared_ptr<PowerDevice> power;

std::shared_ptr<PowerDevice> getOrCreatePower() {
    if (power == nullptr) {
        power = std::make_shared<Xpt2046Power>();
    }
    return power;
}

