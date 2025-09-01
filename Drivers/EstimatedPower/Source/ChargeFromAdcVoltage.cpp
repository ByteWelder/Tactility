#include "ChargeFromAdcVoltage.h"
#include <Tactility/Log.h>
#include <algorithm>

constexpr auto TAG = "EstimatePower";
constexpr auto MAX_VOLTAGE_SAMPLES = 15;

uint8_t ChargeFromAdcVoltage::estimateChargeLevelFromVoltage(uint32_t milliVolt) const {
    float volts = std::min((float)milliVolt / 1000.f, configuration.batteryVoltageMax);
    float voltage_percentage = (volts - configuration.batteryVoltageMin) / (configuration.batteryVoltageMax - configuration.batteryVoltageMin);
    float voltage_factor = std::min(1.0f, voltage_percentage);
    auto charge_level = (uint8_t) (voltage_factor * 100.f);
    TT_LOG_V(TAG, "mV = %lu, scaled = %.2f, factor = %.2f, result = %d", milliVolt, volts, voltage_factor, charge_level);
    return charge_level;
}

ChargeFromAdcVoltage::ChargeFromAdcVoltage(const Configuration& configuration) : configuration(configuration) {
    if (adc_oneshot_new_unit(&this->configuration.adcConfig, &adcHandle) != ESP_OK) {
        TT_LOG_E(TAG, "ADC config failed");
        return;
    }

    if (adc_oneshot_config_channel(adcHandle, ADC_CHANNEL_3, &this->configuration.adcChannelConfig) != ESP_OK) {
        TT_LOG_E(TAG, "ADC channel config failed");

        adc_oneshot_del_unit(adcHandle);
        return;
    }
}

ChargeFromAdcVoltage::~ChargeFromAdcVoltage() {
    if (adcHandle) {
        adc_oneshot_del_unit(adcHandle);
    }
}

bool ChargeFromAdcVoltage::readBatteryVoltageOnce(uint32_t& output) {
    int raw;
    if (adc_oneshot_read(adcHandle, ADC_CHANNEL_3, &raw) == ESP_OK) {
        output = configuration.adcMultiplier * ((1000.f * configuration.adcRefVoltage) / 4096.f) * (float)raw;
        TT_LOG_V(TAG, "Raw = %d, voltage = %lu", raw, output);
        return true;
    } else {
        TT_LOG_E(TAG, "Read failed");
        return false;
    }
}

bool ChargeFromAdcVoltage::readBatteryVoltageSampled(uint32_t& output) {
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
