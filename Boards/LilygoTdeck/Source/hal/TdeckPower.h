#pragma once

#include <Tactility/hal/Power.h>
#include <esp_adc/adc_oneshot.h>
#include <memory>

using namespace tt::hal;

class TdeckPower : public Power {

    adc_oneshot_unit_handle_t adcHandle = nullptr;

public:

    TdeckPower();
    ~TdeckPower();

    bool supportsMetric(MetricType type) const override;
    bool getMetric(Power::MetricType type, Power::MetricData& data) override;

private:

    bool readBatteryVoltageSampled(uint32_t& output);
    bool readBatteryVoltageOnce(uint32_t& output);
};

std::shared_ptr<Power> tdeck_get_power();
