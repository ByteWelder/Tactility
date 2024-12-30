#pragma once

#include "hal/Power.h"
#include <memory>

using namespace tt::hal;

class Core2Power : public Power {

private:

    bool allowedToCharge = true;

public:

    Core2Power() = default;
    ~Core2Power() override = default;

    bool supportsMetric(MetricType type) const override;
    bool getMetric(Power::MetricType type, Power::MetricData& data) override;

    bool supportsChargeControl() const override { return true; }
    bool isAllowedToCharge() const override;
    void setAllowedToCharge(bool canCharge) override;
};

std::shared_ptr<Power> createPower();
