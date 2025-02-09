#pragma once

#include "Tactility/hal/power/PowerDevice.h"
#include <memory>

using tt::hal::power::PowerDevice;

class UnPhonePower : public PowerDevice {

public:

    UnPhonePower() = default;
    ~UnPhonePower() = default;

    std::string getName() const final { return "XPT2046 Power Measurement"; }
    std::string getDescription() const final { return "Power interface via XPT2046 voltage measurement"; }

    bool supportsMetric(MetricType type) const override;
    bool getMetric(MetricType type, MetricData& data) override;

private:

    bool readBatteryVoltageOnce(uint32_t& output) const;
    bool readBatteryVoltageSampled(uint32_t& output) const;
};

std::shared_ptr<PowerDevice> unPhoneGetPower();
