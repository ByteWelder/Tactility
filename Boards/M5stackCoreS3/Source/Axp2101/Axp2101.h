#pragma once

#include "hal/i2c/I2c.h"

/**
 * References:
 * - https://github.com/m5stack/M5Unified/blob/master/src/utility/AXP2101_Class.cpp
 * - http://file.whycan.com/files/members/6736/AXP2101_Datasheet_V1.0_en_3832.pdf
 */
class Axp2101 {

    i2c_port_t port;

    static constexpr uint8_t DEFAULT_ADDRESS = 0x34;
    static constexpr TickType_t DEFAULT_TIMEOUT = 1000 / portTICK_PERIOD_MS;

    bool readRegister8(uint8_t reg, uint8_t& result) const;
    bool writeRegister8(uint8_t reg, uint8_t value) const;
    bool readRegister12(uint8_t reg, float& out) const;
    bool readRegister14(uint8_t reg, float& out) const;
    bool readRegister16(uint8_t reg, uint16_t& out) const;

public:

    enum ChargeStatus {
        CHARGE_STATUS_CHARGING = 0b01,
        CHARGE_STATUS_DISCHARGING = 0b10,
        CHARGE_STATUS_STANDBY = 0b00
    };

    Axp2101(i2c_port_t port) : port(port) {}

    bool getBatteryVoltage(float& vbatMillis) const;
    bool getChargeStatus(ChargeStatus& status) const;
    bool isChargingEnabled(bool& enabled) const;
    bool setChargingEnabled(bool enabled) const;
};
