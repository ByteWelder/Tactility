#pragma once

#include <Tactility/hal/power/Power.h>
#include <esp_adc/adc_oneshot.h>
#include <memory>

using tt::hal::power::Power;

class TdeckPower : public Power {

    adc_oneshot_unit_handle_t adcHandle = nullptr;

public:

    TdeckPower();
    ~TdeckPower();

    std::string getName() const final { return "ADC Power Measurement"; }
    std::string getDescription() const final { return "Power measurement interface via ADC pin"; }

    bool supportsMetric(Power::MetricType type) const override;
    bool getMetric(Power::MetricType type, Power::MetricData& data) override;

private:

    bool readBatteryVoltageSampled(uint32_t& output);
    bool readBatteryVoltageOnce(uint32_t& output);
};

std::shared_ptr<Power> tdeck_get_power();
