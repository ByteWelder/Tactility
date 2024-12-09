#pragma once

#include "hal/Power.h"
#include <memory>

using namespace tt::hal;

class M5stackPower : public Power {

public:

    M5stackPower() {}
    ~M5stackPower() {}

    bool supportsMetric(MetricType type) const override;
    bool getMetric(Power::MetricType type, Power::MetricData& data) override;

    bool supportsChargeControl() const { return true; }
    bool isAllowedToCharge() const { return true; } /** We can call setChargingAllowed() but the actual value is unknown as it resets when re-plugging USB */
    void setAllowedToCharge(bool canCharge);
};

std::shared_ptr<Power> m5stack_get_power();
