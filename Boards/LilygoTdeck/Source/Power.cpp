#include "hal/Power.h"
#include "Log.h"
#include "CoreDefines.h"

#include <esp_adc/adc_continuous.h>
#include <esp_adc/adc_oneshot.h>

#define TAG "power"

// From https://github.com/meshtastic/firmware/blob/f81d3b045dd1b7e3ca7870af3da915ff4399ea98/variants/t-deck/variant.h
// ratio of voltage divider = 2.0 (RD2=100k, RD3=100k)
#define ADC_MULTIPLIER 2.11f // 0.11 added to correct for display under-voltage
#define ADC_REF_VOLTAGE 3.3f
#define BATTERY_VOLTAGE_MIN 3.2f
#define BATTERY_VOLTAGE_MAX 4.2f

adc_oneshot_unit_handle_t adcHandle;
adc_oneshot_unit_init_cfg_t adcConfig = {
    .unit_id = ADC_UNIT_1,
    .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
    .ulp_mode = ADC_ULP_MODE_DISABLE,
};

adc_oneshot_chan_cfg_t adcChannelConfig = {
    .atten = ADC_ATTEN_DB_12,
    .bitwidth = ADC_BITWIDTH_DEFAULT,
};

bool init_power_adc() {
    if (adc_oneshot_new_unit(&adcConfig, &adcHandle) != ESP_OK) {
        TT_LOG_E(TAG, "ADC config failed");
        return false;
    }

    if (adc_oneshot_config_channel(adcHandle, ADC_CHANNEL_3, &adcChannelConfig) != ESP_OK) {
        TT_LOG_E(TAG, "ADC channel config failed");
        return false;
    }

    return true;
}

static bool is_charging() {
    return false;
}

static bool is_charging_enabled() {
    return false;
}

static void set_charging_enabled(bool enabled) {
}

static uint8_t get_charge_level() {
    int raw;
    if (adc_oneshot_read(adcHandle, ADC_CHANNEL_3, &raw) == ESP_OK) {
        float raw_factor = (float)raw / 4096.f;
        float raw_voltage = raw_factor * ADC_REF_VOLTAGE;
        float scaled_voltage = raw_voltage * ADC_MULTIPLIER;
        float voltage_percentage = (scaled_voltage - BATTERY_VOLTAGE_MIN) / (BATTERY_VOLTAGE_MAX - BATTERY_VOLTAGE_MIN);
        float voltage_factor = TT_MIN(1.0f, voltage_percentage);
        auto result = (uint8_t)(voltage_factor * 255.f);
        TT_LOG_W(TAG, "Raw = %d, raw factor %.4f, raw voltage = %.2f, scaled voltage = %.2f, factor = %.2f, result = %d", raw, raw_factor, raw_voltage, scaled_voltage, voltage_factor, result);
        return result;
    } else {
        TT_LOG_E(TAG, "Read failed");
        return 0;
    }
}

static int32_t get_current() {
    return 0;
}

extern const tt::hal::Power tdeck_power = {
    .isCharging = &is_charging,
    .isChargingEnabled = &is_charging_enabled,
    .setChargingEnabled = &set_charging_enabled,
    .getChargeLevel = &get_charge_level,
    .getCurrent = &get_current
};
