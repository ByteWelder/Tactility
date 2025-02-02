#pragma once

#include "Axp2101/Axp2101.h"

#include <Tactility/hal/Power.h>
#include <memory>

using namespace tt::hal;

class CoreS3Power : public Power {

    Axp2101 axpDevice = Axp2101(I2C_NUM_0);

public:

    CoreS3Power() = default;
    ~CoreS3Power() override = default;

    std::string getName() const final { return "AXP2101 Power"; }
    std::string getDescription() const final { return "Power management via I2C"; }

    bool supportsMetric(MetricType type) const override;
    bool getMetric(Power::MetricType type, Power::MetricData& data) override;

    bool supportsChargeControl() const override { return true; }
    bool isAllowedToCharge() const override;
    void setAllowedToCharge(bool canCharge) override;
};

std::shared_ptr<Power> createPower();
