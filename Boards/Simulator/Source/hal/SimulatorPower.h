#pragma once

#include "hal/Power.h"
#include <memory>

using namespace tt::hal;

class SimulatorPower : public Power {

    bool allowedToCharge = false;

public:

    SimulatorPower() {}
    ~SimulatorPower() {}

    bool supportsMetric(MetricType type) const override;
    bool getMetric(Power::MetricType type, Power::MetricData& data) override;

    bool supportsChargeControl() const { return true; }
    bool isAllowedToCharge() const { return allowedToCharge; }
    void setAllowedToCharge(bool canCharge) { allowedToCharge = canCharge; }
};

std::shared_ptr<Power> simulatorPower();
