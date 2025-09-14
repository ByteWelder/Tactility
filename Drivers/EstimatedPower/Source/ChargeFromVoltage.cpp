#include "ChargeFromVoltage.h"
#include <Tactility/Log.h>

constexpr auto* TAG = "ChargeFromVoltage";

uint8_t ChargeFromVoltage::estimateCharge(uint32_t milliVolt) const {
    const float volts = std::min((float)milliVolt / 1000.f, batteryVoltageMax);
    if (volts < batteryVoltageMin) {
        return 0;
    }
    const float voltage_percentage = (volts - batteryVoltageMin) / (batteryVoltageMax - batteryVoltageMin);
    const float voltage_factor = std::min(1.0f, voltage_percentage);
    const auto charge_level = (uint8_t) (voltage_factor * 100.f);
    TT_LOG_D(TAG, "mV = %lu, scaled = %.2f, factor = %.2f, result = %d", milliVolt, volts, voltage_factor, charge_level);
    return charge_level;
}
