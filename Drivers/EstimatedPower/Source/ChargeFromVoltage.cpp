#include "ChargeFromVoltage.h"

#include <Tactility/Logger.h>
#include <algorithm>

const static auto LOGGER = tt::Logger("ChargeFromVoltage");

uint8_t ChargeFromVoltage::estimateCharge(uint32_t milliVolt) const {
    const float volts = std::min((float)milliVolt / 1000.f, batteryVoltageMax);
    if (volts < batteryVoltageMin) {
        return 0;
    }
    const float voltage_percentage = (volts - batteryVoltageMin) / (batteryVoltageMax - batteryVoltageMin);
    const float voltage_factor = std::min(1.0f, voltage_percentage);
    const auto charge_level = (uint8_t) (voltage_factor * 100.f);
    LOGGER.debug("mV = {}, scaled = {}, factor = {:.2f}, result = {}", milliVolt, volts, voltage_factor, charge_level);
    return charge_level;
}
