#pragma once

#include <Tactility/hal/Power.h>
#include <memory>

using namespace tt::hal;

class SimulatorPower final : public Power {

    bool allowedToCharge = false;

public:

    SimulatorPower() = default;
    ~SimulatorPower() override = default;

    std::string getName() const final { return "Power Mock"; }
    std::string getDescription() const final { return ""; }

    bool supportsMetric(MetricType type) const override;
    bool getMetric(Power::MetricType type, Power::MetricData& data) override;

    bool supportsChargeControl() const override { return true; }
    bool isAllowedToCharge() const override { return allowedToCharge; }
    void setAllowedToCharge(bool canCharge) override { allowedToCharge = canCharge; }
};

std::shared_ptr<Power> simulatorPower();
