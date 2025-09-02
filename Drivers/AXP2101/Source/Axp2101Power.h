#pragma once

#include "Tactility/hal/power/PowerDevice.h"
#include <Axp2101.h>
#include <memory>
#include <utility>

using tt::hal::power::PowerDevice;

class Axp2101Power final : public PowerDevice {

    std::shared_ptr<Axp2101> axpDevice;

public:

    explicit Axp2101Power(std::shared_ptr<Axp2101> axp) : axpDevice(std::move(axp)) {}
    ~Axp2101Power() override = default;

    std::string getName() const override { return "AXP2101 Power"; }
    std::string getDescription() const override { return "Power management via AXP2101 over I2C"; }

    bool supportsMetric(MetricType type) const override;
    bool getMetric(MetricType type, MetricData& data) override;

    bool supportsChargeControl() const override { return true; }
    bool isAllowedToCharge() const override;
    void setAllowedToCharge(bool canCharge) override;
};
