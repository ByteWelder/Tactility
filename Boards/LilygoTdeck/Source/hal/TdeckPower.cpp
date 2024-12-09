#include "TdeckPower.h"

#include "Log.h"
#include "CoreDefines.h"

#define TAG "power"

/**
 * The ratio of the voltage divider is supposedly 2.0, but when we set that, the ADC reports a bit over 4.45V
 * when charging the device.
 * There was also supposedly a +0.11 sag compensation related to "display under-voltage" according to Meshtastic firmware.
 * Either Meshtastic implemented it incorrectly OR there is simply a 5-10% deviation in accuracy.
 * The latter is feasible as the selected resistors for the voltage divider might not have been matched appropriately.
 */
#define ADC_MULTIPLIER 1.89f

#define ADC_REF_VOLTAGE 3.3f
#define BATTERY_VOLTAGE_MIN 3.2f
#define BATTERY_VOLTAGE_MAX 4.2f

static adc_oneshot_unit_init_cfg_t adcConfig = {
    .unit_id = ADC_UNIT_1,
    .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
    .ulp_mode = ADC_ULP_MODE_DISABLE,
};

static adc_oneshot_chan_cfg_t adcChannelConfig = {
    .atten = ADC_ATTEN_DB_12,
    .bitwidth = ADC_BITWIDTH_DEFAULT,
};

static uint8_t estimateChargeLevelFromVoltage(uint32_t milliVolt) {
    float volts = TT_MIN((float)milliVolt / 1000.f, BATTERY_VOLTAGE_MAX);
    float voltage_percentage = (volts - BATTERY_VOLTAGE_MIN) / (BATTERY_VOLTAGE_MAX - BATTERY_VOLTAGE_MIN);
    float voltage_factor = TT_MIN(1.0f, voltage_percentage);
    auto charge_level = (uint8_t) (voltage_factor * 100.f);
    TT_LOG_V(TAG, "mV = %lu, scaled = %.2f, factor = %.2f, result = %d", milliVolt, volts, voltage_factor, charge_level);
    return charge_level;
}

TdeckPower::TdeckPower() {
    if (adc_oneshot_new_unit(&adcConfig, &adcHandle) != ESP_OK) {
        TT_LOG_E(TAG, "ADC config failed");
        return;
    }

    if (adc_oneshot_config_channel(adcHandle, ADC_CHANNEL_3, &adcChannelConfig) != ESP_OK) {
        TT_LOG_E(TAG, "ADC channel config failed");

        adc_oneshot_del_unit(adcHandle);
        return;
    }
}

TdeckPower::~TdeckPower() {
    if (adcHandle) {
        adc_oneshot_del_unit(adcHandle);
    }
}

bool TdeckPower::supportsMetric(MetricType type) const {
    switch (type) {
        case BATTERY_VOLTAGE:
        case CHARGE_LEVEL:
            return true;
        case IS_CHARGING:
        case CURRENT:
            return false;
    }

    return false; // Safety guard for when new enum values are introduced
}

bool TdeckPower::getMetric(Power::MetricType type, Power::MetricData& data) {
    switch (type) {
        case BATTERY_VOLTAGE:
            return readBatteryVoltageSampled(data.valueAsUint32);
        case CHARGE_LEVEL:
            if (readBatteryVoltageSampled(data.valueAsUint32)) {
                data.valueAsUint32 = estimateChargeLevelFromVoltage(data.valueAsUint32);
                return true;
            } else {
                return false;
            }
        case IS_CHARGING:
        case CURRENT:
            return false;
    }

    return false; // Safety guard for when new enum values are introduced
}

bool TdeckPower::readBatteryVoltageOnce(uint32_t& output) {
    int raw;
    if (adc_oneshot_read(adcHandle, ADC_CHANNEL_3, &raw) == ESP_OK) {
        output = ADC_MULTIPLIER * ((1000.f * ADC_REF_VOLTAGE) / 4096.f) * (float)raw;
        TT_LOG_V(TAG, "Raw = %d, voltage = %lu", raw, output);
        return true;
    } else {
        TT_LOG_E(TAG, "Read failed");
        return false;
    }
}

#define MAX_VOLTAGE_SAMPLES 15

bool TdeckPower::readBatteryVoltageSampled(uint32_t& output) {
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

std::shared_ptr<Power> tdeck_get_power() {
    if (power == nullptr) {
        power = std::make_shared<TdeckPower>();
    }
    return power;
}
