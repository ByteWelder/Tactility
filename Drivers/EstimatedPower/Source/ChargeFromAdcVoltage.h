#pragma once

#include <esp_adc/adc_oneshot.h>

class ChargeFromAdcVoltage {

public:

    struct Configuration {
        float adcMultiplier = 1.0f;
        float adcRefVoltage = 3.3f;
        float batteryVoltageMin = 3.2f;
        float batteryVoltageMax = 4.2f;
        adc_oneshot_unit_init_cfg_t adcConfig = {
            .unit_id = ADC_UNIT_1,
            .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
            .ulp_mode = ADC_ULP_MODE_DISABLE,
        };
        adc_oneshot_chan_cfg_t adcChannelConfig = {
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
    };

private:

    adc_oneshot_unit_handle_t adcHandle = nullptr;
    Configuration configuration;

public:

    explicit ChargeFromAdcVoltage(const Configuration& configuration);

    ~ChargeFromAdcVoltage();

    uint8_t estimateChargeLevelFromVoltage(uint32_t milliVolt) const;

    bool readBatteryVoltageSampled(uint32_t& output);

    bool readBatteryVoltageOnce(uint32_t& output);
};
