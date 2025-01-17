#pragma once

#include "hal/i2c/I2c.h"

class I2cDevice {

protected:

    i2c_port_t port;
    uint8_t address;

    static constexpr TickType_t DEFAULT_TIMEOUT = 1000 / portTICK_PERIOD_MS;

    bool readRegister8(uint8_t reg, uint8_t& result) const;
    bool writeRegister8(uint8_t reg, uint8_t value) const;
    bool readRegister12(uint8_t reg, float& out) const;
    bool readRegister14(uint8_t reg, float& out) const;
    bool readRegister16(uint8_t reg, uint16_t& out) const;
    bool bitOn(uint8_t reg, uint8_t bitmask) const;
    bool bitOff(uint8_t reg, uint8_t bitmask) const;

public:

    explicit I2cDevice(i2c_port_t port, uint32_t address) : port(port), address(address) {}
};
