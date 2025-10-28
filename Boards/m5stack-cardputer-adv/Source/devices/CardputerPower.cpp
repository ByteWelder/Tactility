#include "CardputerPower.h"

#include <Tactility/Log.h>
#include <driver/adc.h>

constexpr auto* TAG = "CardputerPower";

bool CardputerPower::adcInitCalibration() {
    bool calibrated = false;

    esp_err_t efuse_read_result = esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP_FIT);
    if (efuse_read_result == ESP_ERR_NOT_SUPPORTED) {
        TT_LOG_W(TAG, "Calibration scheme not supported, skip software calibration");
    } else if (efuse_read_result == ESP_ERR_INVALID_VERSION) {
        TT_LOG_W(TAG, "eFuse not burnt, skip software calibration");
    } else if (efuse_read_result == ESP_OK) {
        calibrated = true;
        TT_LOG_I(TAG, "Calibration success");
        esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, static_cast<adc_bits_width_t>(ADC_WIDTH_BIT_DEFAULT), 0, &adcCharacteristics);
    } else {
        TT_LOG_W(TAG, "eFuse read failed, skipping calibration");
    }

    return calibrated;
}

uint32_t CardputerPower::adcReadValue() const {
    int adc_raw = adc1_get_raw(ADC1_CHANNEL_9);
    TT_LOG_D(TAG, "Raw  data: %d", adc_raw);
    float voltage;
    if (calibrated) {
        voltage = esp_adc_cal_raw_to_voltage(adc_raw, &adcCharacteristics);
        TT_LOG_D(TAG, "Calibrated data: %d mV", voltage);
    } else {
        voltage = 0.0f;
    }
    return voltage;
}

bool CardputerPower::ensureInitialized() {
    if (!initialized) {
        calibrated = adcInitCalibration();

        if (adc1_config_width(static_cast<adc_bits_width_t>(ADC_WIDTH_BIT_DEFAULT)) != ESP_OK) {
            TT_LOG_E(TAG, "ADC1 config width failed");
            return false;
        }
        if (adc1_config_channel_atten(ADC1_CHANNEL_9, ADC_ATTEN_DB_11) != ESP_OK) {
            TT_LOG_E(TAG, "ADC1 config attenuation failed");
            return false;
        }

        initialized = true;
    }

    return true;
}

bool CardputerPower::supportsMetric(MetricType type) const {
    switch (type) {
        using enum MetricType;
        case BatteryVoltage:
        case ChargeLevel:
            return true;
        default:
            return false;
    }
}

bool CardputerPower::getMetric(MetricType type, MetricData& data) {
    if (!ensureInitialized()) {
        return false;
    }

    switch (type) {
        case MetricType::BatteryVoltage:
            data.valueAsUint32 = adcReadValue() * 2;
            return true;
        case MetricType::ChargeLevel:
            data.valueAsUint8 = chargeFromAdcVoltage.estimateCharge(adcReadValue() * 2);
            return true;
        default:
            return false;
    }
}
