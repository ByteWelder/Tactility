#pragma once

#include <Tactility/hal/power/PowerDevice.h>

class Xpt2046Touch;
using tt::hal::power::PowerDevice;

/**
 * Power management based on the voltage measurement at the LCD panel.
 * This estimates the battery power left.
 */
class Xpt2046Power : public PowerDevice {

    std::shared_ptr<Xpt2046Touch> xptTouch;

    bool readBatteryVoltageOnce(uint32_t& output);
    bool readBatteryVoltageSampled(uint32_t& output);

public:

    ~Xpt2046Power() override = default;

    std::string getName() const final { return "XPT2046 Power Measurement"; }
    std::string getDescription() const final { return "Power interface via XPT2046 voltage measurement"; }

    bool supportsMetric(MetricType type) const override;
    bool getMetric(MetricType type, MetricData& data) override;
};
