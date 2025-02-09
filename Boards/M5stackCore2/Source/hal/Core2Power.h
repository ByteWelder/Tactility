#pragma once

#include <Tactility/hal/power/Power.h>
#include <memory>

using tt::hal::power::Power;

class Core2Power : public Power {

public:

    Core2Power() = default;
    ~Core2Power() override = default;

    std::string getName() const final { return "AXP192 Power"; }
    std::string getDescription() const final { return "Power management via I2C"; }

    bool supportsMetric(MetricType type) const override;
    bool getMetric(Power::MetricType type, Power::MetricData& data) override;

    bool supportsChargeControl() const override { return true; }
    bool isAllowedToCharge() const override;
    void setAllowedToCharge(bool canCharge) override;
};

std::shared_ptr<Power> createPower();
