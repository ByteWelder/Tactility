#pragma once

#include <Tactility/hal/Power.h>
#include <memory>

using namespace tt::hal;

class SimulatorPower : public Power {

    bool allowedToCharge = false;

public:

    SimulatorPower() = default;
    ~SimulatorPower() override = default;

    bool supportsMetric(MetricType type) const override;
    bool getMetric(Power::MetricType type, Power::MetricData& data) override;

    bool supportsChargeControl() const override { return true; }
    bool isAllowedToCharge() const override { return allowedToCharge; }
    void setAllowedToCharge(bool canCharge) override { allowedToCharge = canCharge; }
};

std::shared_ptr<Power> simulatorPower();
