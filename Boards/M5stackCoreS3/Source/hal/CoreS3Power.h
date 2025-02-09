#pragma once

#include "Tactility/hal/power/PowerDevice.h"
#include <Axp2101.h>
#include <memory>
#include <utility>

using tt::hal::power::PowerDevice;

class CoreS3Power final : public PowerDevice {

    std::shared_ptr<Axp2101> axpDevice;

public:

    explicit CoreS3Power(std::shared_ptr<Axp2101> axp) : axpDevice(std::move(axp)) {}
    ~CoreS3Power() override = default;

    std::string getName() const final { return "AXP2101 Power"; }
    std::string getDescription() const final { return "Power management via I2C"; }

    bool supportsMetric(MetricType type) const override;
    bool getMetric(MetricType type, MetricData& data) override;

    bool supportsChargeControl() const override { return true; }
    bool isAllowedToCharge() const override;
    void setAllowedToCharge(bool canCharge) override;
};

std::shared_ptr<PowerDevice> createPower();
