#pragma once

#include <ChargeFromAdcVoltage.h>
#include <Tactility/hal/power/PowerDevice.h>

using tt::hal::power::PowerDevice;

/**
 * Uses Voltage measurements to estimate charge.
 * Supports voltage and charge level metrics.
 * Can be overridden to further extend supported metrics.
 */
class EstimatedPower final : public PowerDevice {

    std::unique_ptr<ChargeFromAdcVoltage> chargeFromAdcVoltage;

public:

    explicit EstimatedPower(ChargeFromAdcVoltage::Configuration configuration) :
        chargeFromAdcVoltage(std::make_unique<ChargeFromAdcVoltage>(std::move(configuration))) {}

    std::string getName() const override { return "ADC Power Measurement"; }
    std::string getDescription() const override { return "Power measurement interface via ADC pin"; }

    bool supportsMetric(MetricType type) const override;
    bool getMetric(MetricType type, MetricData& data) override;
};
