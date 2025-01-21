#pragma once

#include "hal/Power.h"
#include <memory>

using namespace tt::hal;

class UnPhonePower : public Power {

public:

    UnPhonePower() = default;
    ~UnPhonePower() = default;

    bool supportsMetric(MetricType type) const override;
    bool getMetric(Power::MetricType type, Power::MetricData& data) override;

private:

    bool readBatteryVoltageOnce(uint32_t& output) const;
    bool readBatteryVoltageSampled(uint32_t& output) const;
};

std::shared_ptr<Power> unPhoneGetPower();
