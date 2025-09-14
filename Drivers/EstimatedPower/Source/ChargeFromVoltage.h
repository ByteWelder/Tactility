#pragma once

#include <cstdint>

class ChargeFromVoltage {

    float batteryVoltageMin;
    float batteryVoltageMax;

public:

    explicit ChargeFromVoltage(float voltageMin = 3.2f, float voltageMax = 4.2f) :
        batteryVoltageMin(voltageMin),
        batteryVoltageMax(voltageMax)
    {}

    /**
     * @param milliVolt
     * @return a value in the rage of [0, 100] which represents [0%, 100%] charge
     */
    uint8_t estimateCharge(uint32_t milliVolt) const;
};
