#pragma once

#include <Tactility/hal/power/PowerDevice.h>
#include <memory>

using tt::hal::power::PowerDevice;

class SimulatorPower final : public PowerDevice {

    bool allowedToCharge = false;

public:

    SimulatorPower() = default;
    ~SimulatorPower() override = default;

    std::string getName() const override { return "Power Mock"; }
    std::string getDescription() const override { return ""; }

    bool supportsMetric(MetricType type) const override;
    bool getMetric(MetricType type, MetricData& data) override;

    bool supportsChargeControl() const override { return true; }
    bool isAllowedToCharge() const override { return allowedToCharge; }
    void setAllowedToCharge(bool canCharge) override { allowedToCharge = canCharge; }
};
