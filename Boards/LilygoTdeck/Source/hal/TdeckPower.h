#pragma once

#include <Tactility/hal/power/PowerDevice.h>
#include <esp_adc/adc_oneshot.h>

using tt::hal::power::PowerDevice;

class TdeckPower final : public PowerDevice {

    adc_oneshot_unit_handle_t adcHandle = nullptr;

public:

    TdeckPower();
    ~TdeckPower();

    std::string getName() const final { return "ADC Power Measurement"; }
    std::string getDescription() const final { return "Power measurement interface via ADC pin"; }

    bool supportsMetric(MetricType type) const override;
    bool getMetric(MetricType type, MetricData& data) override;

private:

    bool readBatteryVoltageSampled(uint32_t& output);
    bool readBatteryVoltageOnce(uint32_t& output);
};
