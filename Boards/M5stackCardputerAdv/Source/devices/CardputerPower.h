#pragma once

#include <Tactility/hal/power/PowerDevice.h>
#include <ChargeFromVoltage.h>
#include <esp_adc_cal.h>
#include <string>

using tt::hal::power::PowerDevice;

class CardputerPower final : public PowerDevice {

    ChargeFromVoltage chargeFromAdcVoltage = ChargeFromVoltage(3.3f, 4.2f);
    bool initialized = false;
    esp_adc_cal_characteristics_t adcCharacteristics;
    bool calibrated = false;

    bool adcInitCalibration();
    uint32_t adcReadValue() const;

    bool ensureInitialized();

public:

    std::string getName() const override { return "Cardputer Power"; }
    std::string getDescription() const override { return "Power measurement via ADC"; }

    bool supportsMetric(MetricType type) const override;
    bool getMetric(MetricType type, MetricData& data) override;
};
