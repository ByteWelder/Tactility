#pragma once

#include "I2cDevice/I2cDevice.h"

#define AW9523_ADDRESS 0x58

class Aw9523 : I2cDevice {

public:

    explicit Aw9523(i2c_port_t port) : I2cDevice(port, AW9523_ADDRESS) {}

    bool readP0(uint8_t& output) const;
    bool readP1(uint8_t& output) const;

    bool writeP0(uint8_t value) const;
    bool writeP1(uint8_t value) const;

    bool bitOnP1(uint8_t bitmask) const;
};
