#include "hal/Power.h"
#include "Log.h"

#include <esp_adc/adc_continuous.h>
#include <esp_adc/adc_oneshot.h>

#define TAG "power"

// From https://github.com/meshtastic/firmware/blob/f81d3b045dd1b7e3ca7870af3da915ff4399ea98/variants/t-deck/variant.h
// ratio of voltage divider = 2.0 (RD2=100k, RD3=100k)
//#define ADC_MULTIPLIER 2.11 // 2.0 + 10% for correction of display under voltage

adc_oneshot_unit_handle_t adcHandle;
adc_oneshot_unit_init_cfg_t adcConfig = {
    .unit_id = ADC_UNIT_1,
    .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
    .ulp_mode = ADC_ULP_MODE_DISABLE,
};

adc_oneshot_chan_cfg_t adcChannelConfig = {
    .atten = ADC_ATTEN_DB_0,
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

    int value;
    if (adc_oneshot_read(adcHandle, ADC_CHANNEL_3, &value) == ESP_OK) {
        float scaled = value / 4096.f * 255.f;
        TT_LOG_W(TAG, "Read %d (%f scaled)", value, scaled);
        return scaled;
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
