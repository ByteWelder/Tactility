#pragma once

#include "Tactility/hal/power/PowerDevice.h"
#include <memory>

using tt::hal::power::PowerDevice;

class Core2Power : public PowerDevice {

public:

    Core2Power() = default;
    ~Core2Power() override = default;

    std::string getName() const final { return "AXP192 Power"; }
    std::string getDescription() const final { return "Power management via I2C"; }

    bool supportsMetric(MetricType type) const override;
    bool getMetric(MetricType type, MetricData& data) override;

    bool supportsChargeControl() const override { return true; }
    bool isAllowedToCharge() const override;
    void setAllowedToCharge(bool canCharge) override;
};

std::shared_ptr<PowerDevice> createPower();
